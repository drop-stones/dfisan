#include "UseDefAnalysis/UseDefSVFIRBuilder.h"

#include "llvm/Support/Debug.h"
#define DEBUG_TYPE "usedef-svfir-builder"

using namespace SVF;

// TODO: Support global struct zeroinitializer
void UseDefSVFIRBuilder::addGlobalStructZeroInitializer(const llvm::GlobalVariable *BaseVal, llvm::StructType *StructTy, const llvm::Constant *StructInit, unsigned &AccumulateOffset) {
  LLVM_DEBUG(llvm::dbgs() << __func__ << "\n");
  SVFIR *Pag = getPAG();
  NodeIDAllocator *IDAllocator = NodeIDAllocator::get();
  for (auto *EleTy : StructTy->elements()) {
    LLVM_DEBUG(llvm::dbgs() << "EleTy: " << *EleTy << "\n");

    if (auto *EleStructTy = SVFUtil::dyn_cast<StructType>(EleTy)) {
      addGlobalStructZeroInitializer(BaseVal, EleStructTy, StructInit, AccumulateOffset);
    } else {
      NodeID FieldID = getGlobalVarField(BaseVal, AccumulateOffset, EleTy);
      const auto *FieldNode = Pag->getGNode(FieldID);
      LLVM_DEBUG(llvm::dbgs() << "Create FieldInitNode: " << FieldNode->toString() << "\n");

      NodeID ZeroInitID = IDAllocator->allocateValueId();
      Pag->addValNode(StructInit, ZeroInitID);
      auto *Stmt = Pag->addStoreStmt(ZeroInitID, FieldID, nullptr);
      Stmt->setICFGNode(Pag->getICFG()->getGlobalICFGNode());
      Stmt->setValue(StructInit);
      LLVM_DEBUG(llvm::dbgs() << "Add StoreStmt: " << Stmt->toString() << "\n");
    
      AccumulateOffset++;
    }
  }
}

void UseDefSVFIRBuilder::addGlobalStructMemberInitializer(const llvm::GlobalVariable *BaseVal, llvm::StructType *StructTy, const llvm::ConstantStruct *StructConst, unsigned &AccumulateOffset, llvm::SmallVector<unsigned, 8> &OffsetVec) {
  LLVM_DEBUG(llvm::dbgs() << __func__ << "\n");
  SVFIR *Pag = getPAG();
  NodeIDAllocator *IDAllocator = NodeIDAllocator::get();
  if (StructConst->isZeroValue()) {
    addGlobalStructZeroInitializer(BaseVal, StructTy, StructConst, AccumulateOffset);
    return;
  }
  for (auto *EleTy : StructTy->elements()) {
    LLVM_DEBUG(llvm::dbgs() << "EleTy: " << *EleTy << "\n");
    unsigned &Offset = OffsetVec.back();

    if (auto *EleStructTy = SVFUtil::dyn_cast<StructType>(EleTy)) {
      const auto *EleStructConst = SVFUtil::dyn_cast<ConstantStruct>(StructConst->getAggregateElement(Offset));
      OffsetVec.push_back(0);
      addGlobalStructMemberInitializer(BaseVal, EleStructTy, EleStructConst, AccumulateOffset, OffsetVec);
      OffsetVec.pop_back();
      Offset++;
    } else if (auto *EleArrayTy = SVFUtil::dyn_cast<ArrayType>(EleTy)) {
      NodeID ArrayID = getGlobalVarField(BaseVal, AccumulateOffset, EleTy);
      const auto *ArrayNode = Pag->getGNode(ArrayID);
      LLVM_DEBUG(llvm::dbgs() << "Create ArrayInitNode: " << ArrayNode->toString() << "\n");

      const auto *ArrayInit = StructConst->getAggregateElement(Offset);
      NodeID ArrayInitID = IDAllocator->allocateValueId();
      assert(ArrayInit != nullptr);
      LLVM_DEBUG(llvm::dbgs() << "ArrayInit(Offset=" << AccumulateOffset << "): " << *ArrayInit << "\n");
      Pag->addValNode(ArrayInit, ArrayInitID);
      auto *Stmt = Pag->addStoreStmt(ArrayInitID, ArrayID, nullptr);
      Stmt->setICFGNode(Pag->getICFG()->getGlobalICFGNode());
      Stmt->setValue(ArrayInit);
      LLVM_DEBUG(llvm::dbgs() << "Add StoreStmt: " << Stmt->toString() << "\n");

      Offset++;
      AccumulateOffset++;
    } else {
      Offset++;
      AccumulateOffset++;
    }
  }
}

// TODO: Support array in struct
void UseDefSVFIRBuilder::addGlobalAggregateTypeInitializationNodes() {
  LLVM_DEBUG(llvm::dbgs() << __func__ << "\n");
  SVFIR *Pag = getPAG();
  NodeIDAllocator *IDAllocator = NodeIDAllocator::get();
  llvm::SmallVector<const AddrStmt *, 8> ArrayVec;
  llvm::SmallVector<const AddrStmt *, 8> StructVec;
  for (const auto *GlobalStmt : Pag->getGlobalSVFStmtSet()) {
    if (!SVFUtil::isa<AddrStmt>(GlobalStmt))
      continue;
    const AddrStmt *Addr = SVFUtil::dyn_cast<AddrStmt>(GlobalStmt);
    const auto *Val = Addr->getValue();
    if (Val == nullptr || !SVFUtil::isa<llvm::GlobalVariable>(Val))
      continue;
    const auto *GlobalVar = SVFUtil::dyn_cast<llvm::GlobalVariable>(Val);
    const auto *VarTy = GlobalVar->getValueType();
    if (VarTy == nullptr || (!SVFUtil::isa<llvm::ArrayType>(VarTy) && !SVFUtil::isa<llvm::StructType>(VarTy)))
      continue;
    LLVM_DEBUG(llvm::dbgs() << "Aggregate Var: " << *GlobalVar << "\n");
    if (SVFUtil::isa<llvm::ArrayType>(VarTy))
      ArrayVec.push_back(Addr);
    if (SVFUtil::isa<llvm::StructType>(VarTy))
      StructVec.push_back(Addr);
  }

  for (const auto *ArrayStmt : ArrayVec) {  // Add array initialization nodes
    const auto *ArrayVal = SVFUtil::dyn_cast<llvm::GlobalVariable>(ArrayStmt->getValue());
    const auto *ArrayTy = SVFUtil::dyn_cast<llvm::ArrayType>(ArrayVal->getValueType());
    const auto *ArrayInit = ArrayVal->getInitializer();
    LLVM_DEBUG(llvm::dbgs() << "Array: " << *ArrayVal << "\n");
    LLVM_DEBUG(llvm::dbgs() << "ArrayTy: " << *ArrayTy << "\n");
    LLVM_DEBUG(llvm::dbgs() << "ArrayInit: " << *ArrayInit << "\n");

    NodeID ArrayID = getValueNode(ArrayVal);
    const auto *ArrayNode = Pag->getGNode(ArrayID);
    LLVM_DEBUG(llvm::dbgs() << "ArrayNode: " << ArrayNode->toString() << "\n");

    NodeID ArrayInitID = IDAllocator->allocateValueId();
    Pag->addValNode(ArrayInit, ArrayInitID);
    auto *Stmt = Pag->addStoreStmt(ArrayInitID, ArrayID, nullptr);
    Stmt->setICFGNode(Pag->getICFG()->getGlobalICFGNode());
    Stmt->setValue(ArrayInit);
    LLVM_DEBUG(llvm::dbgs() << "StoreStmt: " << Stmt->toString() << "\n");
  }

  for (const auto *StructStmt : StructVec) {  // Add struct initialize nodes
    const auto *StructVal = SVFUtil::dyn_cast<llvm::GlobalVariable>(StructStmt->getValue());
    auto *StructTy = SVFUtil::dyn_cast<llvm::StructType>(StructVal->getValueType());
    const auto *StructInit = StructVal->getInitializer();
    LLVM_DEBUG(llvm::dbgs() << "Struct: " << *StructVal << "\n");
    LLVM_DEBUG(llvm::dbgs() << "StructTy: " << *StructTy << "\n");
    LLVM_DEBUG(llvm::dbgs() << "StructInit: " << *StructInit << "\n");

    if (StructInit->isZeroValue()) {  // zero initializer
      unsigned AccumulateOffset = 0;
      addGlobalStructZeroInitializer(StructVal, StructTy, StructInit, AccumulateOffset);
    } else {  // array members
      if (!SVFUtil::isa<llvm::ConstantStruct>(StructInit))
        continue;
      const auto *StructConst = SVFUtil::dyn_cast<llvm::ConstantStruct>(StructInit);
      LLVM_DEBUG(llvm::dbgs() << "StructConst: " << *StructConst << "\n");

      unsigned AccumulateOffset = 0;
      llvm::SmallVector<unsigned, 8> OffsetVec { 0 };
      addGlobalStructMemberInitializer(StructVal, StructTy, StructConst, AccumulateOffset, OffsetVec);
    }
  }
}