#include "Passes/DfiReadWriteGraphBuilder.h"
// #include "dg/Passes/DfiUtils.h"

namespace dg {
namespace dda {

RWNode *DfiReadWriteGraphBuilder::createLoad(const llvm::Instruction *Inst) {
    RWNode *ret = LLVMReadWriteGraphBuilder::createLoad(Inst);

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

    return ret;
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
      DefSite Site{&GlobalNode};
      GlobalNode.addDef(Site);
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

void DfiReadWriteGraphBuilder::buildSubgraph(const llvm::Function &F) {
    LLVMReadWriteGraphBuilder::buildSubgraph(F);

    // Insert global variable initializations into entry of main.
    if (F.getName() == "main") {
        auto *Main = getSubgraph(&F);
        auto *MainEntryBlock = Main->getBBlocks().begin()->get();
        for (auto *GlobalNode : getGlobals()) {
            if (GlobalNode->isDef())
                MainEntryBlock->prepend(GlobalNode);
        }
    }
}

} // namespace dda
} // namespace dg