#include "Passes/DfiReadWriteGraphBuilder.h"
#include "dg/Passes/DfiOptions.h"
#include "llvm/ReadWriteGraph/LLVMReadWriteGraphBuilder.h"
#include "llvm/llvm-utils.h"

namespace dg {
namespace dda {

/* --- dg/llvm/ReadWriteGraph/Calls.cpp --- */
template <typename T>
std::pair<Offset, Offset> getFromTo(const llvm::CallInst *CInst, T what) {
    auto from = what->from.isOperand()
                        ? llvmutils::getConstantValue(
                                  CInst->getArgOperand(what->from.getOperand()))
                        : what->from.getOffset();
    auto to = what->to.isOperand()
                      ? llvmutils::getConstantValue(
                                CInst->getArgOperand(what->to.getOperand()))
                      : what->to.getOffset();

    return {from, to};
}
std::pair<Offset, Offset> getFromToOfVararg(const llvm::CallInst *CInst, const dg::FunctionModel::Operand *Ope) {
    assert(Ope->from.isOperand() && "Vararg's from must be operand");
    Offset From = Ope->from.getOperand();
    assert(Ope->to.isOffset() && Ope->to.getOffset() == Offset::UNKNOWN && "Vararg's to must be Offset::UNKNOWN");
    Offset To = Ope->to.getOffset();
    return {From, To};
}

RWNode *DfiReadWriteGraphBuilder::funcFromModel(const FunctionModel *Model, const llvm::CallInst *CInst) {
    auto *Node = LLVMReadWriteGraphBuilder::funcFromModel(Model, CInst);

    // Definitions of vararg
    if (Model->handles(VARARG)) {
        Offset Start, NoUse;
        std::tie(Start, NoUse) = getFromToOfVararg(CInst, Model->defines(VARARG));
        for (unsigned int Idx = Start.offset; Idx < llvmutils::getNumArgOperands(CInst); Idx++) {
            auto const *llvmOp = CInst->getArgOperand(Idx);
            auto Pts = PTA->getLLVMPointsTo(llvmOp);
            for (const auto &Ptr : Pts) {
                RWNode *Target = getOperand(Ptr.value);
                Node->addDef(Target, Ptr.offset, Offset::getUnknown());
            }
        }
    }

    // Definitions of the return value.
    if (Model->handles(RETURN)) {
        if (const auto *Defines = Model->defines(RETURN)) {
            Offset From, To;
            std::tie(From, To) = getFromTo(CInst, Defines);
            for (const auto &Ptr : PTA->getLLVMPointsTo(CInst)) {
                RWNode *Target = getOperand(Ptr.value);
                Node->addDef(Target, Ptr.offset + From, Ptr.offset + To);
            }
        }
    }

    return Node;
}

/* --- dg/llvm/ReadWriteGraph/Instructions.cpp --- */
RWNode *DfiReadWriteGraphBuilder::createLoad(const llvm::Instruction *Inst) {
    RWNode *ret = LLVMReadWriteGraphBuilder::createLoad(Inst);

/*
    /// Print for debug
    auto &Loc = Inst->getDebugLoc();
    if (Loc) {
        if (Loc->getColumn() == 20 && Loc->getLine() == 671) {
            llvm::errs() << "Found " << *Inst << "\n";
            for (const auto &Use : ret->getUses()) {
                llvm::errs() << " - " << *getValue(Use.target) << "\n";
            }
        }
    }
*/

    return ret;
}

// Add calloc def
RWNode *DfiReadWriteGraphBuilder::createDynAlloc(const llvm::Instruction *Inst, AllocationFunction Type) {
    auto *Node = LLVMReadWriteGraphBuilder::createDynAlloc(Inst, Type);

    // Definitions by calloc
    if (Type == AllocationFunction::CALLOC) {
        uint64_t Size = Node->getSize();
        if (Size != 0) {
            Node->addDef(Node, 0, Size, false);
        } else {
            Node->addDef(Node, 0, Offset::getUnknown(), false);
        }
    }

    return Node;
}

template <typename OptsT>
static bool isRelevantCall(const llvm::Instruction *Inst, OptsT &opts) {
    using namespace llvm;

    // we don't care about debugging stuff
    if (isa<DbgValueInst>(Inst))
        return false;

    const CallInst *CInst = cast<CallInst>(Inst);
#if LLVM_VERSION_MAJOR >= 8
    const Value *calledVal = CInst->getCalledOperand()->stripPointerCasts();
#else
    const Value *calledVal = CInst->getCalledValue()->stripPointerCasts();
#endif
    const Function *func = dyn_cast<Function>(calledVal);

    if (!func)
        // function pointer call - we need that
        return true;

    if (func->empty()) {
        // we have a model for this function
        if (opts.getFunctionModel(func->getName().str()))
            return true;
        // memory allocation
        if (opts.isAllocationFunction(func->getName().str()))
            return true;

        if (func->isIntrinsic()) {
            switch (func->getIntrinsicID()) {
            case Intrinsic::memmove:
            case Intrinsic::memcpy:
            case Intrinsic::memset:
            case Intrinsic::vastart:
                return true;
            default:
                return false;
            }
        }

        // undefined function
        return true;
    } // we want defined function, since those can contain
    // pointer's manipulation and modify CFG
    return true;

    assert(0 && "We should not reach this");
}

NodesSeq<RWNode> DfiReadWriteGraphBuilder::createNode(const llvm::Value *V) {
  using namespace llvm;
  if (isa<GlobalVariable>(V)) {
    // global variables are like allocations
    // and store initial values.
    auto &GlobalNode = create(RWNodeType::GLOBAL);
    if (ProtectInfo->hasTarget((llvm::Value *)V)) {
        GlobalNode.addDef(&GlobalNode, Offset::getUnknown(), Offset::getUnknown());
        assert(GlobalNode.isDef() && "GlobalNode is not def");
    }
    return {&GlobalNode};
  }

  const auto *I = dyn_cast<Instruction>(V);
  if (!I)
      return {};

  // we may created this node when searching for an operand
  switch (I->getOpcode()) {
  case Instruction::Alloca:
      // we need alloca's as target to DefSites
      return {createAlloc(I)};
  case Instruction::Store:
      return {createStore(I)};
  case Instruction::AtomicRMW:
      return {createAtomicRMW(I)};
  case Instruction::Load:
      if (buildUses) {
          return {createLoad(I)};
      }
      break;
  case Instruction::Ret:
      // we need create returns, since
      // these modify CFG and thus data-flow
      return {createReturn(I)};
  case Instruction::Call:
      if (!isRelevantCall(I, _options))
          break;

      return createCall(I);
  }

  return {};
}

/* --- dg/llvm/GraphBuilder.h --- */
void DfiReadWriteGraphBuilder::buildSubgraph(const llvm::Function &F) {
    LLVMReadWriteGraphBuilder::buildSubgraph(F);

    // Insert global variable initializations into entry of main.
    if (F.getName() == "main") {
        auto *Main = getSubgraph(&F);
        auto *MainEntryBlock = Main->getBBlocks().begin()->get();
        for (auto *GlobalNode : getGlobals()) {
            if (GlobalNode->isDef()) {
                MainEntryBlock->prepend(GlobalNode);
            }
        }
    }
}

} // namespace dda
} // namespace dg