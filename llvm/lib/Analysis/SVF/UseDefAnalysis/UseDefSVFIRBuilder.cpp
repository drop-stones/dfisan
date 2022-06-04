#include "UseDefAnalysis/UseDefSVFIRBuilder.h"
#include "SVF-FE/SymbolTableBuilder.h"
#include "Util/Options.h"

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
    auto *ArrayNode = Pag->getGNode(ArrayID);
    LLVM_DEBUG(llvm::dbgs() << "ArrayNode: " << ArrayNode->toString() << "\n");

    NodeID ArrayInitID = IDAllocator->allocateValueId();
    Pag->addValNode(ArrayInit, ArrayInitID);
    Pag->getGNode(ArrayInitID);

    llvm::SmallVector <GepValVar *, 8> GepValVarVec;
    if (ArrayNode->hasOutgoingEdges(SVFStmt::Gep)) {  // array which has gep nodes
      for (auto *OutgoingEdge : ArrayNode->getOutgoingEdges(SVFStmt::Gep)) {
        auto *GepNode = OutgoingEdge->getDstNode();
        if (auto *GepValVarNode = SVFUtil::dyn_cast<GepValVar>(GepNode)) {
          LLVM_DEBUG(llvm::dbgs() << " - GepValVar: " << GepNode->toString() << "\n");
          GepValVarVec.push_back(GepValVarNode);
        }
      }
    }

    if (GepValVarVec.empty()) {   // Array of primitive data type
      auto *Stmt = Pag->addStoreStmt(ArrayInitID, ArrayID, nullptr);
      Stmt->setICFGNode(Pag->getICFG()->getGlobalICFGNode());
      Stmt->setValue(ArrayInit);
      LLVM_DEBUG(llvm::dbgs() << "StoreStmt: " << Stmt->toString() << "\n");
    } else {  // Array of struct
      for (auto *GepValVarNode : GepValVarVec) {
        NodeID FieldID = GepValVarNode->getId();
        auto *FieldNode = Pag->getGNode(FieldID);

        // Remove other store nodes
        /*
        llvm::SmallVector<SVFVar *, 8> ToRemoveStoreVec;
        for (auto *IncomingEdge : FieldNode->getIncomingEdges(SVFStmt::Store)) {
          ToRemoveStoreVec.push_back(IncomingEdge->getSrcNode());
        }
        for (auto *ToRemoveStore : ToRemoveStoreVec) {
          llvm::SmallVector<SVFStmt *, 8> ToRemoveInEdges;
          llvm::SmallVector<SVFStmt *, 8> ToRemoveOutEdges;
          for (auto *IncomingEdge : ToRemoveStore->getInEdges()) {
            ToRemoveInEdges.push_back(IncomingEdge);
            IncomingEdge->getSrcNode()->removeOutgoingEdge(IncomingEdge);
          }
          for (auto *ToRemoveIn : ToRemoveInEdges) {
            ToRemoveStore->removeIncomingEdge(ToRemoveIn);
            LLVM_DEBUG(llvm::dbgs() << "Remove InEdge: " << ToRemoveIn->toString() << "\n");
          }
          for (auto *OutgoingEdge : ToRemoveStore->getOutEdges()) {
            ToRemoveOutEdges.push_back(OutgoingEdge);
            OutgoingEdge->getDstNode()->removeIncomingEdge(OutgoingEdge);
          }
          for (auto *ToRemoveOut : ToRemoveOutEdges) {
            ToRemoveStore->removeOutgoingEdge(ToRemoveOut);
            LLVM_DEBUG(llvm::dbgs() << "Remove OutEdge: " << ToRemoveOut->toString() << "\n");
          }

          LLVM_DEBUG(llvm::dbgs() << "Remove StoreNode: " << ToRemoveStore->toString() << "\n");
          Pag->removeGNode(ToRemoveStore);
        }
        */

        auto *Stmt = Pag->addStoreStmt(ArrayInitID, FieldID, nullptr);
        Stmt->setICFGNode(Pag->getICFG()->getGlobalICFGNode());
        Stmt->setValue(ArrayInit);
        LLVM_DEBUG(llvm::dbgs() << "StoreStmt: " << Stmt->toString() << "\n");
      }
    }
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

/*For global variable initialization
 * Give a simple global variable
 * int x = 10;     // store 10 x  (constant, non pointer)                                      |
 * int *y = &x;    // store x y   (pointer type)
 * Given a struct
 * struct Z { int s; int *t;};
 * Global initialization:
 * struct Z z = {10,&x}; // store x z.t  (struct type)
 * struct Z *m = &z;       // store z m  (pointer type)
 * struct Z n = {10,&z.s}; // store z.s n ,  &z.s constant expression (constant expression)
 * struct Z l;    // store zeroinitializer {l.s, l.t}
 */
void UseDefSVFIRBuilder::InitialGlobal(const GlobalVariable *Gvar, Constant *Const, u32_t Offset) {
  SVFIR *Pag = getPAG();
  SymbolTableInfo *SymInfo = SymbolTableInfo::SymbolInfo();
  if (Const->getType()->isSingleValueType()) {
    NodeID Src = getValueNode(Const);
    // get the field value if it is avaiable, otherwise we create a dummy field node.
    setCurrentLocation(Gvar, nullptr);
    NodeID Field = getGlobalVarField(Gvar, Offset, Const->getType());

    if (SVFUtil::isa<GlobalVariable>(Const) || SVFUtil::isa<Function>(Const)) {
      LLVM_DEBUG(llvm::dbgs() << "Single GlobalVariable: " << *Const << "\n");
      setCurrentLocation(Const, nullptr);
      addStoreEdge(Src, Field);
    } else if (SVFUtil::isa<ConstantExpr>(Const)) {
      // add gep edge of C1 itself is a constant expression
      LLVM_DEBUG(llvm::dbgs() << "ConstantExpr: " << *Const << "\n");
      processCE(Const);
      setCurrentLocation(Const, nullptr);
      addStoreEdge(Src, Field);
    } else if (SVFUtil::isa<BlockAddress>(Const)) {
      // blockaddress instruction (e.g. i8* blockaddress(@run_vm, %182))
      // is treated as constant data object for now, see LLVMUtil.h:397, SymbolTableInfo.cpp:674 and SVFIRBuilder.cpp:183-194
      LLVM_DEBUG(llvm::dbgs() << "BlockAddress: " << *Const << "\n");
      processCE(Const);
      setCurrentLocation(Const, nullptr);
      addAddrEdge(Pag->getConstantNode(), Src);
    } else {
      LLVM_DEBUG(llvm::dbgs() << "Single Global Value Type: " << *Const << "\n");
      setCurrentLocation(Const, nullptr);
      addStoreEdge(Src, Field);
      /// Src should not point to anything yet
      if (Const->getType()->isPtrOrPtrVectorTy() && Src != Pag->getNullPtr())
        addCopyEdge(Pag->getNullPtr(), Src);
    }
  } else if (SVFUtil::isa<ConstantArray>(Const) || SVFUtil::isa<ConstantStruct>(Const)) {
    LLVM_DEBUG(llvm::dbgs() << "Constant Aggregate Type: " << *Const << "\n");
    for (u32_t I = 0, E = Const->getNumOperands(); I != E; I++){
      u32_t Off = SymInfo->getFlattenedElemIdx(Const->getType(), I);
      InitialGlobal(Gvar, SVFUtil::cast<Constant>(Const->getOperand(I)), Offset + Off);
    }
  }
  /// New implementation
  else if(ConstantData* Data = SVFUtil::dyn_cast<ConstantData>(Const)) {    // Create zeroinitializer PAG nodes
    LLVM_DEBUG(llvm::dbgs() << "Const Data: " << *Data << "\n");
    if (!Data->isZeroValue())
      return;

    const Type *DataTy = Data->getType();
    for (u32_t Idx = 0; Idx < SymInfo->getNumOfFlattenElements(DataTy); Idx++) {
      auto *ElemTy = (Type *)SymInfo->getFlatternedElemType(DataTy, Idx);
      LLVM_DEBUG(llvm::dbgs() << "ElemTy: " << *ElemTy << "\n");

      if (!Pag->hasValueNode(Data)) {   /// Create new ValueNode
        // Add new symbol
        SymbolTableBuilder Builder {SymInfo};
        Builder.collectVal(Data);

        // Add new pag node
        setCurrentLocation(Data, nullptr);
        NodeID SrcID = SymInfo->getValSym(Data);
        Pag->addValNode(Data, SrcID);

        LLVM_DEBUG(llvm::dbgs() << "SymTableBuilder.collectVal(): " << *Const << "\n");
        LLVM_DEBUG(llvm::dbgs() << "hasValueNode = " << Pag->hasValueNode(Const) << "\n");
        LLVM_DEBUG(llvm::dbgs() << "addValNode(" << SrcID << "): " << Pag->getGNode(SrcID)->toString() << "\n");
      }

      NodeID SrcID = getValueNode(Const);
      setCurrentLocation(Gvar, nullptr);
      NodeID FieldID = getGlobalVarField(Gvar, Offset + Idx, ElemTy);

      auto *Field = Pag->getGNode(FieldID);
      LLVM_DEBUG(llvm::dbgs() << "FieldID(" << FieldID << "): " << Field->toString() << "\n");
      auto *Src = Pag->getGNode(SrcID);
      LLVM_DEBUG(llvm::dbgs() << "SrcID(" << SrcID << "): " << Src->toString() << "\n");

      setCurrentLocation(Const, nullptr);
      addStoreEdge(SrcID, FieldID);
      /// Src should not point to anything yet
      if (Const->getType()->isPtrOrPtrVectorTy() && SrcID != Pag->getNullPtr()) {
        addCopyEdge(Pag->getNullPtr(), SrcID);
      }
    }
  }
  else{
    //TODO:assert(SVFUtil::isa<ConstantVector>(Const),"what else do we have");
  }
}
