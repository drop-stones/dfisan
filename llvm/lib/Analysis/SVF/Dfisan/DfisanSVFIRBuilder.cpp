#include "Dfisan/DfisanSVFIRBuilder.h"
#include "Dfisan/DfisanExtAPI.h"
#include "SVF-FE/LLVMUtil.h"
#include "Util/Options.h"

using namespace SVF;
using namespace SVFUtil;
using namespace LLVMUtil;

static const Type *getTypeAndFlattenedFields(const Value *V, std::vector<LocationSet> &fields) {
  assert(V);
  const Type *Ty = V->getType();
  while (const PointerType *PtrTy = SVFUtil::dyn_cast<PointerType>(Ty)) {
    Ty = getPtrElementType(PtrTy);
  }

  u32_t numOfElems = SymbolTableInfo::SymbolInfo()->getNumOfFlattenElements(Ty);
  LLVMContext &Ctx = LLVMModuleSet::getLLVMModuleSet()->getContext();
  for (u32_t ei = 0; ei < numOfElems; ei++) {
    LocationSet ls(ei);
    const ConstantInt *Offset = ConstantInt::get(Ctx, llvm::APInt(32, ei));
    ls.addOffsetValue(Offset, nullptr);
    fields.push_back(ls);
  }
  return Ty;
}

NodeID DfisanSVFIRBuilder::getZeroValNode() {
  LLVMContext &Ctx = LLVMModuleSet::getLLVMModuleSet()->getContext();
  const ConstantInt *ZeroVal = ConstantInt::get(Ctx, llvm::APInt(32, 0));
  return getPAG()->addValNode(ZeroVal, NodeIDAllocator::get()->allocateValueId());
}

void DfisanSVFIRBuilder::addComplexConsForDfisanExt(NodeID Dst, NodeID Src) {
  std::vector<LocationSet> Fields;
  const Value *Val = getPAG()->getGNode(Dst)->getValue();
  const Type *Ty = getTypeAndFlattenedFields(Val, Fields);
  for (u32_t Idx = 0; Idx < Fields.size(); Idx++) {
    const Type *ElemTy = SymbolTableInfo::SymbolInfo()->getFlatternedElemType(Ty, Fields[Idx].accumulateConstantFieldIdx());
    NodeID ElemField = getGepValVar(Val, Fields[Idx], ElemTy);
    addStoreEdge(Src, ElemField);
  }
}

// Return base value: we need override because of memcpy and memset handling
//   - for local var: alloca
//   - for heap var : bitcast (not malloc because we cannot get correct type)
const Value *DfisanSVFIRBuilder::getBaseValueForExtArg(const Value *V) {
    const Value * value = stripAllCasts(V);
    assert(value && "null ptr?");
    if (const GetElementPtrInst* gep = SVFUtil::dyn_cast<GetElementPtrInst>(value))
    {
      s32_t totalidx = 0;
      for (bridge_gep_iterator gi = bridge_gep_begin(gep), ge = bridge_gep_end(gep); gi != ge; ++gi)
      {
        if(const ConstantInt *op = SVFUtil::dyn_cast<ConstantInt>(gi.getOperand()))
            totalidx += op->getSExtValue();
      }
      if(totalidx == 0 && !SVFUtil::isa<StructType>(value->getType()))
        value = gep->getPointerOperand();
    } else if (const CallInst *Call = SVFUtil::dyn_cast<CallInst>(value)) {
      // malloc handling
      const auto *NextInst = Call->getNextNonDebugInstruction();
      if (SVFUtil::isa<BitCastInst>(NextInst))
        value = NextInst;
    }
    return value;
}

/// Handle external calls
///   + Dfisan's external calls
void DfisanSVFIRBuilder::handleExtCall(CallSite cs, const SVFFunction *F) {
  SVFIRBuilder::handleExtCall(cs, F);
  if (!isExtCall(F))  return;

  /// DfisanExtAPI handling
  auto *dfisanExtAPI = DfisanExtAPI::getDfisanExtAPI();
  if (dfisanExtAPI->isExtDefFun(F)) {
    const auto &ExtFun = dfisanExtAPI->getExtFun(F);
    NodeID DstID, ValID;
    if (ExtFun.Ty == DfisanExtAPI::ExtFunType::EXT_CALLOC) {
      const Instruction *Dst = cs.getInstruction();
      if (SVFUtil::isa<BitCastInst>(Dst->getNextNonDebugInstruction())) // calloc's return must be bitcasted.
        Dst = Dst->getNextNonDebugInstruction();
      DstID = getValueNode(Dst);
      ValID = getZeroValNode();
      addComplexConsForDfisanExt(DstID, ValID);
    } else if (ExtFun.Pos == DfisanExtAPI::AccessPosition::RET) {
      const Instruction *Dst = cs.getInstruction();
      DstID = getValueNode(Dst);
      ValID = getZeroValNode();
      addComplexConsForDfisanExt(DstID, ValID);
    } else if (ExtFun.Pos == DfisanExtAPI::AccessPosition::ARG) {
      const CallInst *Call = SVFUtil::dyn_cast<CallInst>(cs.getInstruction());
      const Value *Dst = stripAllCasts(Call->getArgOperand(ExtFun.ArgPos));
      DstID = getValueNode(Dst);
      ValID = getZeroValNode();
      addComplexConsForDfisanExt(DstID, ValID);
    } else if (ExtFun.Pos == DfisanExtAPI::AccessPosition::VARARG) {
      const CallInst *Call = SVFUtil::dyn_cast<CallInst>(cs.getInstruction());
      for (unsigned Idx = ExtFun.ArgPos; Idx < Call->arg_size(); ++Idx) {
        const Value *Dst = stripAllCasts(Call->getArgOperand(Idx));
        DstID = getValueNode(Dst);
        ValID = getZeroValNode();
        addComplexConsForDfisanExt(DstID, ValID);
      }
    } else {
      llvm_unreachable("DfisanExtAPI: No support api");
    }
  }
  if (dfisanExtAPI->isExtUseFun(F)) {
    // TODO
  }
}

/// Handle global init
void DfisanSVFIRBuilder::InitialGlobal(const GlobalVariable *gvar, Constant *C, u32_t offset) {
  InitialGlobalForDfisan(gvar, C, offset);
}

/// Impl of global init handling
void DfisanSVFIRBuilder::InitialGlobalForDfisan(const GlobalVariable *gvar, Constant *C, u32_t offset, Constant *Base) {
  if (C->getType()->isSingleValueType()) {
    NodeID src = getValueNode(C);
    // NodeID src = (Base == nullptr) ? getValueNode(C) : getValueNode(Base);

    // get the field value if it is avaiable, otherwise we create a dummy field node.
    setCurrentLocation(gvar, nullptr);
    NodeID field = getGlobalVarField(gvar, offset, C->getType());

    if (SVFUtil::isa<GlobalVariable>(C) || SVFUtil::isa<Function>(C)) {
      //if (Base != nullptr)
      //  setCurrentLocation(Base, nullptr);
      //else
      setCurrentLocation(C, nullptr);
      addStoreEdge(src, field);
    } else if (SVFUtil::isa<ConstantExpr>(C)) {
      // add gep edge of C1 itself is a constant expression
      // if (Base != nullptr) {
      //   processCE(Base);
      //   setCurrentLocation(Base, nullptr);
      // } else {
      //   processCE(C);
      //   setCurrentLocation(C, nullptr);
      // }
      processCE(C);
      setCurrentLocation(C, nullptr);
      addStoreEdge(src, field);
    } else if (SVFUtil::isa<BlockAddress>(C)) {
      // blockaddress instruction (e.g. i8* blockaddress(@run_vm, %182))
      // is treated as constant data object for now, see LLVMUtil.h:397, SymbolTableInfo.cpp:674 and SVFIRBuilder.cpp:183-194
      // if (Base != nullptr) {
      //   processCE(Base);
      //   setCurrentLocation(Base, nullptr);
      // } else {
      //   processCE(C);
      //   setCurrentLocation(C, nullptr);
      // }
      processCE(C);
      setCurrentLocation(C, nullptr);
      addAddrEdge(getPAG()->getConstantNode(), src);
    } else {
      // if (Base != nullptr)
      //   setCurrentLocation(Base, nullptr);
      // else
      setCurrentLocation(C, nullptr);
      addStoreEdge(src, field);
      /// src should not point to anything yet
      if (C->getType()->isPtrOrPtrVectorTy() && src != getPAG()->getNullPtr())
        addCopyEdge(getPAG()->getNullPtr(), src);
    }
  } else if (SVFUtil::isa<ConstantArray>(C) || SVFUtil::isa<ConstantStruct>(C)) {
    for (u32_t i = 0, e = C->getNumOperands(); i != e; i++) {
      u32_t off = SymbolTableInfo::SymbolInfo()->getFlattenedElemIdx(C->getType(), i);
      /* Global init support */
      // InitialGlobal(gvar, SVFUtil::cast<Constant>(C->getOperand(i)), offset + off);
      if (!getPAG()->hasValueNode(C)) {
        NodeID ID = NodeIDAllocator::get()->allocateValueId();
        SymbolTableInfo::SymbolInfo()->valSyms().insert(std::make_pair(C, ID));
        getPAG()->addValNode(C, ID);
      }
      if (Base == nullptr)
        InitialGlobalForDfisan(gvar, SVFUtil::cast<Constant>(C->getOperand(i)), offset + off, C);
      else
        InitialGlobalForDfisan(gvar, SVFUtil::cast<Constant>(C->getOperand(i)), offset + off, Base);
    }
  } else if (ConstantData* data = SVFUtil::dyn_cast<ConstantData>(C)) {
    if (Options::ModelConsts) {
      if (ConstantDataSequential* seq = SVFUtil::dyn_cast<ConstantDataSequential>(data)) {
        for (u32_t i = 0; i < seq->getNumElements(); i++) {
          u32_t off = SymbolTableInfo::SymbolInfo()->getFlattenedElemIdx(C->getType(), i);
          Constant* ct = seq->getElementAsConstant(i);
          InitialGlobal(gvar, ct, offset + off);
        }
      } else {
        assert((SVFUtil::isa<ConstantAggregateZero>(data) || SVFUtil::isa<UndefValue>(data)) && "Single value type data should have been handled!");
      }
    }
  } else {
    //TODO:assert(SVFUtil::isa<ConstantVector>(C),"what else do we have");
  }

  /* Array support */
  if (SVFUtil::isa<ArrayType>(gvar->getValueType())) {
    assert(C != nullptr && "Constant is nullptr");
    // FIXME: Adding value node with Constant *C causes errors at VFG::getDef() "SVFVar does not have a definition??".
    // NodeID src = getPAG()->addValNode(C, NodeIDAllocator::get()->allocateValueId());
    NodeID src = getZeroValNode();
    NodeID dst = getValueNode(gvar);
    addStoreEdge(src, dst);
  }
}
