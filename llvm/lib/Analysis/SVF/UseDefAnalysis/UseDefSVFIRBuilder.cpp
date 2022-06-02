#include "UseDefAnalysis/UseDefSVFIRBuilder.h"

#include "llvm/Support/Debug.h"
#define DEBUG_TYPE "usedef-svfir-builder"

using namespace SVF;

// TODO: Support global struct zeroinitializer
void UseDefSVFIRBuilder::addGlobalStructZeroInitializer(SVFIR *Pag, const llvm::GlobalVariable *GlobalVar) {
  LLVM_DEBUG(llvm::dbgs() << __func__ << "\n");
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

  for (const auto *StructStmt : StructVec) {  // Add struct zero initializer nodes
    const auto *StructVal = SVFUtil::dyn_cast<llvm::GlobalVariable>(StructStmt->getValue());
    const auto *StructTy = SVFUtil::dyn_cast<llvm::StructType>(StructVal->getValueType());
    const auto *StructInit = StructVal->getInitializer();
    LLVM_DEBUG(llvm::dbgs() << "Struct: " << *StructVal << "\n");
    LLVM_DEBUG(llvm::dbgs() << "StructTy: " << *StructTy << "\n");
    LLVM_DEBUG(llvm::dbgs() << "StructInit: " << *StructInit << "\n");

    if (StructInit->isZeroValue()) {  // zero initializer
      for (unsigned EleOffset = 0; EleOffset < StructTy->getNumElements(); EleOffset++) {
        auto *EleTy = StructTy->getElementType(EleOffset);
        LLVM_DEBUG(llvm::dbgs() << "EleTy: " << *EleTy << "\n");

        NodeID FieldID = getGlobalVarField(StructVal, EleOffset, EleTy);
        const auto *FieldNode = Pag->getGNode(FieldID);
        LLVM_DEBUG(llvm::dbgs() << "Create Field: " << FieldNode->toString() << "\n");

        NodeID ZeroInitID = IDAllocator->allocateValueId();
        Pag->addValNode(StructInit, ZeroInitID);
        auto *Stmt = Pag->addStoreStmt(ZeroInitID, FieldID, nullptr);
        Stmt->setICFGNode(Pag->getICFG()->getGlobalICFGNode());
        Stmt->setValue(StructInit);
        LLVM_DEBUG(llvm::dbgs() << "StoreStmt: " << Stmt->toString() << "\n");
      }
    }
  }
}