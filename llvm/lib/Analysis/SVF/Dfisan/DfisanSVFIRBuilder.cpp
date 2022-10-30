#include "Dfisan/DfisanSVFIRBuilder.h"
#include "Dfisan/DfisanExtAPI.h"
#include "SVF-FE/LLVMUtil.h"

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

/// Handle external calls
///   + Dfisan's external calls
void DfisanSVFIRBuilder::handleExtCall(CallSite cs, const SVFFunction *F) {
  SVFIRBuilder::handleExtCall(cs, F);
  if (!isExtCall(F))  return;

  auto *dfisanExtAPI = DfisanExtAPI::getDfisanExtAPI();
  if (dfisanExtAPI->isExtDefFun(F)) {
    const auto &ExtFun = dfisanExtAPI->getExtFun(F);
    NodeID DstID, ValID;
    if (ExtFun.Pos == DfisanExtAPI::AccessPosition::RET) {
      const Instruction *Dst = cs.getInstruction();
      if (SVFUtil::isa<BitCastInst>(Dst->getNextNonDebugInstruction())) // calloc's return must be bitcasted.
        Dst = Dst->getNextNonDebugInstruction();
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
  SVFIRBuilder::InitialGlobal(gvar, C, offset);
  // Support global array init
  if (SVFUtil::isa<ArrayType>(gvar->getValueType())) {
    assert(C != nullptr && "Constant is nullptr");
    // FIXME: Adding value node with Constant *C causes errors at VFG::getDef() "SVFVar does not have a definition??".
    // NodeID src = getPAG()->addValNode(C, NodeIDAllocator::get()->allocateValueId());
    NodeID src = getZeroValNode();
    NodeID dst = getValueNode(gvar);
    addStoreEdge(src, dst);
  }
}
