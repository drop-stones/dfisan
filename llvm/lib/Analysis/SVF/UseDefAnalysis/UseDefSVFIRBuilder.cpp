#include "UseDefAnalysis/UseDefSVFIRBuilder.h"
#include "SVF-FE/SymbolTableBuilder.h"
#include "Util/Options.h"

#include "llvm/Support/Debug.h"
#define DEBUG_TYPE "usedef-svfir-builder"

using namespace SVF;

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
  else if(ConstantData* Data = SVFUtil::dyn_cast<ConstantData>(Const)) {    // Create aggregate data initialize PAG nodes
    LLVM_DEBUG(llvm::dbgs() << "Const Data: " << *Data << "\n");

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
