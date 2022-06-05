//===-- UseDefChain.cpp - Use-Def Chain implementation --------------------===//
//
//===----------------------------------------------------------------------===//
///
/// This file contains the implementation of the UseDefChain class,
/// which consists of an Use and all reachable Definitions.
///
//===----------------------------------------------------------------------===//

#include "UseDefAnalysis/UseDefChain.h"
#include "MemoryModel/SVFVariables.h"

#include "llvm/Support/Debug.h"
#define DEBUG_TYPE "usedef-chain"

using namespace SVF;
using namespace llvm;

namespace {
const Value *getBase(const Value *FieldVal) {
  if (FieldVal == nullptr)
    return nullptr;

  // The destination must be `bitcast %struct`.
  const Value *Base = nullptr;
  if (const auto *Bitcast = llvm::dyn_cast<const BitCastInst>(FieldVal)) {
    const Value *V = Bitcast->getOperand(0);
    if (llvm::isa<const AllocaInst>(V)) {
      Base = V;
      LLVM_DEBUG(dbgs() << "Base: " << *Base << "\n");
    }
  } else if (const auto *GlobalVal = llvm::dyn_cast<const GlobalValue>(FieldVal)) {
    Base = GlobalVal;
    LLVM_DEBUG(dbgs() << "GlobalValue: " << *GlobalVal << "\n");
  }
  return Base;
}

/// Get Type from Base value.
const Type *getTypeFromBase(const Value *Base) {
  if (Base == nullptr)
    return nullptr;
  
  const Type *BaseTy = nullptr;
  if (const auto *Alloca = llvm::dyn_cast<const AllocaInst>(Base))
    BaseTy = Alloca->getAllocatedType();
  else if (const auto *GlobalVal = llvm::dyn_cast<const GlobalValue>(Base))
    BaseTy = GlobalVal->getValueType();
  
  return BaseTy;
}

const StructType *getStructTypeFromBase(const Value *Base) {
  const StructType *StructTy = nullptr;
  const auto *BaseTy = getTypeFromBase(Base);
  if (BaseTy != nullptr && llvm::isa<const StructType>(BaseTy)) {
    StructTy = llvm::dyn_cast<const StructType>(BaseTy);
    LLVM_DEBUG(dbgs() << "Base StructType: " << *BaseTy << "\n");
  }

  return StructTy;
}

const ArrayType *getArrayTypeFromBase(const Value *Base) {
  const ArrayType *ArrayTy = nullptr;
  const auto *BaseTy = getTypeFromBase(Base);
  if (BaseTy != nullptr && llvm::isa<const ArrayType>(BaseTy)) {
    ArrayTy = llvm::dyn_cast<const ArrayType>(BaseTy);
  }

  return ArrayTy;
}

bool calculateStructOffset(FieldOffsetVector &OffsetVec, const StructType *StructTy, unsigned &RemainOffset, const Value *Base);
bool calculateArrayOffset(FieldOffsetVector &OffsetVec, const ArrayType *ArrayTy, unsigned &RemainOffset, const Value *Base);

// Return true if offset calculation is ended.
bool calculateOffsetVec(FieldOffsetVector &OffsetVec, const Type *BaseTy, unsigned &RemainOffset, const Value *Base) {
  bool Ret = false;
  if (const StructType *StructTy = llvm::dyn_cast<const StructType>(BaseTy)) {
    Ret = calculateStructOffset(OffsetVec, StructTy, RemainOffset, Base);
  } else if (const ArrayType *ArrayTy = llvm::dyn_cast<const ArrayType>(BaseTy)) {
    ArrayOffset *Offset = new ArrayOffset {ArrayTy, Base, 0};
    OffsetVec.push_back(Offset);
    Ret = true;
  }
  return Ret;
}


// Process one element in StructTy or ArrayTy.
bool calculateOffsetVecFromElement(FieldOffsetVector &OffsetVec, const StructType *ParentTy, Type *EleTy, unsigned EleOffset, unsigned &RemainOffset, const Value *Base) {
  if (const auto *EleStructTy = llvm::dyn_cast<const StructType>(EleTy)) {
    // Dive into element struct.
    //StructOffset *Offset = new StructOffset {EleStructTy, Base, EleOffset};
    StructOffset *Offset = new StructOffset {ParentTy, Base, EleOffset};
    OffsetVec.push_back(Offset);
    bool IsEnd = calculateStructOffset(OffsetVec, EleStructTy, RemainOffset, Base);
    if (IsEnd)
      return true;
    OffsetVec.pop_back(); // Back to the parent struct
  //} else if (const auto *EleArrayTy = llvm::dyn_cast<const ArrayType>(EleTy)) {
  //  // Dive into element array.
  //  ArrayOffset *Offset = new ArrayOffset {EleArrayTy, Base, EleOffset};
  //  OffsetVec.push_back(Offset);
  //  bool IsEnd = calculateArrayOffset(OffsetVec, EleArrayTy, RemainOffset, Base);
  //  if (IsEnd)
  //    return true;
  //  OffsetVec.pop_back(); // Back to the parent struct
  //  bool IsEnd = calculateArrayOffset(OffsetVec, EleArrayTy, RemainOffset, Base);
  //  if (IsEnd)
  //    return true;
  } else {
    if (RemainOffset == 0) {
      //if (const auto *ParentStructTy = llvm::dyn_cast<const StructType>(ParentTy)) {
      StructOffset *Offset = new StructOffset {ParentTy, Base, EleOffset};
      OffsetVec.push_back(Offset);
      LLVM_DEBUG(dbgs() << "push_back " << *Offset << "\n");
      //} else if (const auto *ParentArrayTy = llvm::dyn_cast<const ArrayType>(ParentTy)) {
      //  ArrayOffset *Offset = new ArrayOffset {ParentArrayTy, Base, EleOffset};
      //  OffsetVec.push_back(Offset);
      //}
      return true;
    }
    RemainOffset--;
  }
  return false;
}

// Process struct and calculate OffsetVector.
bool calculateStructOffset(FieldOffsetVector &OffsetVec, const StructType *StructTy, unsigned &RemainOffset, const Value *Base) {
  unsigned EleOffset = 0;
  for (auto *EleTy : StructTy->elements()) {
    bool IsEnd = calculateOffsetVecFromElement(OffsetVec, StructTy, EleTy, EleOffset, RemainOffset, Base);
    if (IsEnd == true)
      return true;
    EleOffset++;
  }
  return false;
}

// Process array and calculate OffsetVector.
bool calculateArrayOffset(FieldOffsetVector &OffsetVec, const ArrayType *ArrayTy, unsigned &RemainOffset, const Value *Base) {
  /*
  Type *EleTy = ArrayTy->getElementType();
  LLVM_DEBUG(dbgs() << "ArrayTy->getElementType(): " << *EleTy << "\n");
  for (unsigned EleOffset = 0; EleOffset < ArrayTy->getNumElements(); EleOffset++) {
    // Dive into the element.
    ArrayOffset *Offset = new ArrayOffset {ArrayTy, Base, EleOffset};
    OffsetVec.push_back(Offset);
    bool IsEnd = calculateOffsetVecFromElement(OffsetVec, ArrayTy, EleTy, EleOffset, RemainOffset, Base);
    if (IsEnd == true)
      return true;
    OffsetVec.pop_back();
  }
  */
  return false;
}
} // anonymous namespace

// Dump FieldOffset.
llvm::raw_ostream &SVF::operator<<(llvm::raw_ostream &OS, const FieldOffset &Offset) {
  //OS << "{" << *(Offset.BaseTy) << ", " << Offset.Offset << "}";
  if (const auto *StructOff = llvm::dyn_cast<StructOffset>(&Offset)) {
    OS << *StructOff;
  } else if (const auto *ArrayOff = llvm::dyn_cast<ArrayOffset>(&Offset)) {
    OS << *ArrayOff;
  } else {
    OS << "{" << *(Offset.BaseTy) << ", " << Offset.Offset << "}";
  }
  return OS;
}

// Dump StructOffset.
llvm::raw_ostream &SVF::operator<<(llvm::raw_ostream &OS, const StructOffset &Offset) {
  OS << "{" << *(Offset.StructTy) << ", " << Offset.Offset << "}";
  return OS;
}

// Dump ArrayOffset.
llvm::raw_ostream &SVF::operator<<(llvm::raw_ostream &OS, const ArrayOffset &Offset) {
  OS << "{" << *(Offset.ArrayTy) << ", " << Offset.Offset << "}";
  return OS;
}

void UseDefChain::insert(const LoadSVFGNode *Use, const StoreSVFGNode *Def) {
  UseDef[Use].insert(Def);
}

void UseDefChain::insertDefUsingPtr(const StoreSVFGNode *Def) {
  DefUsingPtrList.insert(Def);
}

void UseDefChain::insertFieldStore(const SVFG *Svfg, const StoreSVFGNode *Def) {
  LLVM_DEBUG(dbgs() << __func__ << ": " << Def->toString() << "\n");
  SVFIR *Pag = Svfg->getPAG();

  // Get the destination node of memcpy.
  const SVFGNode *DstNode = Svfg->getDefSVFGNode(Def->getPAGDstNode());
  if (const auto *GepNode = SVFUtil::dyn_cast<const GepSVFGNode>(DstNode)) {  // If field element
    const auto DefVars = GepNode->getDefSVFVars();
    for (const auto Field : DefVars) {
      // Get the accumulate offset.
      LocationSet Ls = Pag->getLocationSetFromBaseNode(Field);
      auto FieldIdx = Ls.accumulateConstantFieldIdx();
      LLVM_DEBUG(dbgs() << "FieldIdx: " << FieldIdx << "\n");

      // Get the destination field node of memcpy.
      const SVFVar *FieldVar = Pag->getGNode(Field);
      LLVM_DEBUG(dbgs() << "FieldVar: " << FieldVar->toString() << "\n");

      const Value *FieldVal = FieldVar->getValue();
      LLVM_DEBUG(dbgs() << "FieldVal: " << *FieldVal << "\n");
      const Value *Base = getBase(FieldVal);
      LLVM_DEBUG(dbgs() << "Base: " << *Base << "\n");
      const Type *BaseTy = getTypeFromBase(Base);
      LLVM_DEBUG(dbgs() << "BaseTy: " << *BaseTy << "\n");
      FieldOffsetVector OffsetVec;
      unsigned RemainOffset = FieldIdx;
      calculateOffsetVec(OffsetVec, BaseTy, RemainOffset, Base);
      FieldStoreMap.emplace(Def, OffsetVec);
    }
  }
}

void UseDefChain::insertGlobalInit(const StoreSVFGNode *GlobalInit) {
  GlobalInitList.insert(GlobalInit);
}

void UseDefChain::idToUseDef(SVFIR *Pag) {
  // ID to UseDef
  for (const auto &Iter : UseDef) {
    for (const auto *Def : Iter.second) {
      setDefID(Def);
    }
  }

  // ID to DefUsingPtr
  for (const auto *Def : DefUsingPtrList) {
    setDefID(Def);
  }

  // ID to GlobalInit
  for (const auto *GlobalInit : GlobalInitList) {
    setDefID(GlobalInit);
  }

  mergeMemcpyIDs(Pag);
}

void UseDefChain::setDefID(const StoreSVFGNode *Def) {
  static DefID CurrDefID = 1;

  assert(Def != nullptr);

  if (DefToID[Def] == 0) {
    DefToID[Def] = CurrDefID++;
  }

  if (CurrDefID == std::numeric_limits<uint16_t>::max()) {
    llvm::errs() << "DefID overflow occured.\n";
    CurrDefID = 1;
  }
}

const SVFVar *
UseDefChain::getArrayBaseFromMemcpy(SVFIR *Pag, const StoreSVFGNode *StoreNode) {
  const Value *StoreVal = StoreNode->getValue();
  if (StoreVal == nullptr)
    return nullptr;
  if (!llvm::isa<llvm::CallInst>(StoreVal))
    return nullptr;
  const auto *Call = llvm::cast<llvm::CallInst>(StoreVal);
  const auto *Callee = Call->getCalledFunction();
  if (!Callee->isIntrinsic() || !Callee->getName().contains("memcpy"))
    return nullptr;
  LLVM_DEBUG(llvm::dbgs() << "Memcpy: " << *Callee << "\n");
  const PAGNode *DstNode = StoreNode->getPAGDstNode();
  NodeID DstID = StoreNode->getPAGDstNodeID();
  if (DstNode->getNodeKind() == PAGNode::DummyValNode || DstNode->getNodeKind() == PAGNode::DummyObjNode)
    return nullptr;
  if (!llvm::isa<llvm::BitCastInst>(DstNode->getValue()))
    return nullptr;
  const auto *Bitcast = llvm::cast<llvm::BitCastInst>(DstNode->getValue());
  if (!llvm::isa<llvm::PointerType>(Bitcast->getSrcTy()))
    return nullptr;
  const auto *PtrTy = llvm::cast<llvm::PointerType>(Bitcast->getSrcTy());
  if (!llvm::isa<llvm::ArrayType>(PtrTy->getPointerElementType()))
    return nullptr;
  const auto *ArrayTy = llvm::cast<llvm::ArrayType>(PtrTy->getPointerElementType());
  
  NodeID BaseID = Pag->getBaseValVar(StoreNode->getPAGDstNodeID());
  const auto *Base = Pag->getGNode(BaseID);
  return Base;
}

void UseDefChain::mergeMemcpyIDs(SVFIR *Pag) {
  LLVM_DEBUG(llvm::dbgs() << __func__ << "\n");
  using ArrayToMemcpyIDMap = std::unordered_map<const SVFVar *, DefID>;
  ArrayToMemcpyIDMap ArrayBaseToID;
  for (auto Iter : DefToID) {
    const StoreSVFGNode *StoreNode = Iter.first;
    NodeID StoreID = Iter.second;
    const auto *Base = getArrayBaseFromMemcpy(Pag, StoreNode);
    if (Base == nullptr)
      continue;
    if (ArrayBaseToID.count(Base) == 0) {
      ArrayBaseToID[Base] = StoreID;
    } else {
      LLVM_DEBUG(llvm::dbgs() << "Change DefID from " << DefToID[StoreNode] << " to " << ArrayBaseToID[Base] << "\n");
      DefToID[StoreNode] = ArrayBaseToID[Base];
    }
  }
}

void UseDefChain::print(llvm::raw_ostream &OS) const {
  OS << "UseDefChain::print()\n";
  for (const auto &Iter : UseDef) {
    const LoadSVFGNode *Use = Iter.first;
    OS << "- USE:" << *Use->getValue() << "\n";
    for (const auto *Store : Iter.second) {
      DefID ID = getDefID(Store);
      OS << "  - DEF: ID(" << ID << ") " << *(Store->getValue()) << "\n";
      if (containsOffsetVector(Store)) {
        const auto &OffsetVec = getOffsetVector(Store);
        OS << "    - OffsetVec: (";
        for (auto *Offset : OffsetVec)
          OS << *Offset << ", ";
        OS << ")\n";
      }
    }
  }

  OS << "Print Def using pointer\n";
  for (const auto *Store : DefUsingPtrList) {
    DefID ID = getDefID(Store);
    OS << "  - DEF: ID(" << ID << ") " << *(Store->getValue()) << "\n";
  }

  OS << "Print Global initialization\n";
  for (const auto *GlobalInit : GlobalInitList) {
    DefID ID = getDefID(GlobalInit);
    OS << "  - GlobalInit: ID(" << ID << ") " << *(GlobalInit->getValue()) << "\n";
  }
}

UseDefChain::iterator
UseDefChain::begin() {
  return UseDef.begin();
}

UseDefChain::const_iterator
UseDefChain::begin() const {
  return UseDef.begin();
}

UseDefChain::iterator
UseDefChain::end() {
  return UseDef.end();
}

UseDefChain::const_iterator
UseDefChain::end() const {
  return UseDef.end();
}