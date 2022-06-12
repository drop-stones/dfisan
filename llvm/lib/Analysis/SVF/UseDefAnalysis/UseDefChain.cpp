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

  const Value *Base = FieldVal->stripPointerCasts();
  LLVM_DEBUG(dbgs() << "Base: " << *Base << "\n");
  /*
  // The destination must be `bitcast %struct`.
  if (const auto *Bitcast = llvm::dyn_cast<const BitCastInst>(FieldVal)) {
    //Base = Bitcast->getOperand(0);
    Base = FieldVal->stripPointerCasts();
    LLVM_DEBUG(dbgs() << "Base: " << *Base << "\n");
  } else if (const auto *GlobalVal = llvm::dyn_cast<const GlobalValue>(FieldVal)) {
    Base = GlobalVal;
    LLVM_DEBUG(dbgs() << "GlobalValue: " << *GlobalVal << "\n");
  } else {  // If the value is casted to pointer, we get the original value.
    Base = FieldVal->stripPointerCasts();
    //assert(false && "Not supported opcode!! " );
  }
  */
  return Base;
}

/// Get Type from Base value.
const Type *getTypeFromBase(const Value *Base) {
  if (Base == nullptr)
    return nullptr;
  
  const Type *BaseTy = nullptr;
  if (const auto *GlobalVal = llvm::dyn_cast<const GlobalValue>(Base)) {
    BaseTy = GlobalVal->getValueType();
  } else if (const auto *PtrTy = llvm::dyn_cast<llvm::PointerType>(Base->getType())) {
    BaseTy = PtrTy->getPointerElementType();
  } else {
    //assert(false && "Not supported code patterns!!");
  }
  
  return BaseTy;
}

unsigned getBitWidth(const Type *Ty) {
  if (llvm::isa<IntegerType>(Ty))
    return Ty->getIntegerBitWidth();
  if (llvm::isa<ArrayType>(Ty))
    return Ty->getArrayNumElements() * getBitWidth(Ty->getArrayElementType());
  if (llvm::isa<StructType>(Ty)) {
    unsigned BitWidthSum = 0;
    for (unsigned Idx = 0; Idx < Ty->getStructNumElements(); Idx++) {
      BitWidthSum += getBitWidth(Ty->getStructElementType(Idx));
    }
    return BitWidthSum;
  }
  return 0;
}

// Update the current alignment.
void calculateAlign(const Type *Ty, unsigned &CurAlign) {
  unsigned BitWidth = getBitWidth(Ty);
  if (BitWidth >= 32)
    CurAlign = 0;
  else
    CurAlign = (CurAlign + BitWidth) % 32;
}

// Process struct and calculate OffsetVector.
bool calculateStructOffset(FieldOffsetVector &OffsetVec, const StructType *StructTy, unsigned &RemainOffset, const Value *Base, unsigned &CurAlign) {
  SymbolTableInfo *SymInfo = SymbolTableInfo::SymbolInfo();
  auto *StructInfo = SymInfo->getStructInfo(StructTy);
  unsigned FieldIdx = 0;
  for (auto FlattenedFieldIdx : StructInfo->getFlattenedFieldIdxVec()) {
    const auto *FieldTy = StructInfo->getOriginalElemType(FlattenedFieldIdx);
    LLVM_DEBUG(llvm::dbgs() << "- FieldIdx(" << FieldIdx << "): " << *FieldTy << "\n");

    if (const auto *FldStructTy = SVFUtil::dyn_cast<StructType>(FieldTy)) {
      auto *FldStructInfo = SymInfo->getStructInfo(FldStructTy);
      auto NumOfFields = FldStructInfo->getNumOfFlattenFields();
      if (RemainOffset < NumOfFields) {
        StructOffset *Offset = new StructOffset {StructTy, Base, FieldIdx};
        OffsetVec.push_back(Offset);
        LLVM_DEBUG(llvm::dbgs() << "push_back " << *Offset << "\n");
        return calculateStructOffset(OffsetVec, FldStructTy, RemainOffset, Base, CurAlign);
      }
      RemainOffset -= NumOfFields;
    } else {
      if (RemainOffset == 0) {
        if (CurAlign % 32 != 0 && llvm::isa<ArrayType>(FieldTy)) { // Found padding array!!
          LLVM_DEBUG(llvm::dbgs() << "Skip padding array: CurAlign(" << CurAlign << "), Idx(" << FieldIdx << ") " << *FieldTy << "\n");
          OffsetVec.clear();
          return true;
        }
        StructOffset *Offset = new StructOffset {StructTy, Base, FieldIdx};
        OffsetVec.push_back(Offset);
        LLVM_DEBUG(llvm::dbgs() << "push_back " << *Offset << "\n");
        return true;
      }
      calculateAlign(FieldTy, CurAlign);
      RemainOffset--;
    }
    FieldIdx++;
  }

  return false;
}

// Return true if offset calculation is ended.
void calculateOffsetVec(const StoreSVFGNode *Def, FieldOffsetVector &OffsetVec, const Type *BaseTy, unsigned &RemainOffset, const Value *Base) {
  if (const StructType *StructTy = llvm::dyn_cast<const StructType>(BaseTy)) {      // Field-sensitive DefIDs.
    unsigned CurAlign = 0;
    calculateStructOffset(OffsetVec, StructTy, RemainOffset, Base, CurAlign);
  } else if (const ArrayType *ArrayTy = llvm::dyn_cast<const ArrayType>(BaseTy)) {  // Element- and Field-insensitive DefIDs.
    ArrayOffset *Offset = new ArrayOffset {ArrayTy, Base, 0};
    OffsetVec.push_back(Offset);
  } else {                                                                          // Element- and Field-insensitive DefIDS.
    if (const auto *Memcpy = llvm::dyn_cast<const MemCpyInst>(Def->getInst())) {
      const auto *Length = Memcpy->getLength();
      LLVM_DEBUG(dbgs() << "Memcpy->getLength: " << *Length << "\n");
      PointerOffset *Offset = new PointerOffset {Base, Length};
      OffsetVec.push_back(Offset);
    }
  }
}
} // anonymous namespace

// Dump FieldOffset.
llvm::raw_ostream &SVF::operator<<(llvm::raw_ostream &OS, const FieldOffset &Offset) {
  //OS << "{" << *(Offset.BaseTy) << ", " << Offset.Offset << "}";
  if (const auto *StructOff = llvm::dyn_cast<StructOffset>(&Offset)) {
    OS << *StructOff;
  } else if (const auto *ArrayOff = llvm::dyn_cast<ArrayOffset>(&Offset)) {
    OS << *ArrayOff;
  } else if (const auto *PointerOff = llvm::dyn_cast<PointerOffset>(&Offset)) {
    OS << *PointerOff;
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

// Dump PointerOffset.
llvm::raw_ostream &SVF::operator<<(llvm::raw_ostream &OS, const PointerOffset &Offset) {
  OS << "{ i8, " << *(Offset.Length) << "}";
  return OS;
}

void UseDefChain::insert(const LoadSVFGNode *Use, const StoreSVFGNode *Def) {
  if (llvm::isa<UndefValue>(Def->getValue()))
    return;
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

      const SVFVar *FieldVar = Pag->getGNode(Field);
      const Value *Base = getBase(GepNode->getPAGSrcNode()->getValue());
      const Type *BaseTy = getTypeFromBase(Base);
      if (BaseTy == nullptr) {
        dbgs() << "FieldVar: " << FieldVar->toString() << "\n";
        assert(false && "Not supported code pattern!!");
      }
      assert(BaseTy != nullptr);
      LLVM_DEBUG(dbgs() << "FieldVar: " << FieldVar->toString() << "\n");
      LLVM_DEBUG(dbgs() << "Base: " << *Base << "\n");
      LLVM_DEBUG(dbgs() << "BaseTy: " << *BaseTy << "\n");
      FieldOffsetVector OffsetVec;
      unsigned RemainOffset = FieldIdx;
      calculateOffsetVec(Def, OffsetVec, BaseTy, RemainOffset, Base);
      if (OffsetVec.size() == 0) {
        dbgs() << "FieldVar: " << FieldVar->toString() << "\n";
        //assert(false && "OffsetVec is empty!!");
        PaddingFieldSet.insert(FieldVar);
      } else {
        FieldStoreMap.emplace(Def, OffsetVec);
      }
    }
  }
}

void UseDefChain::insertGlobalInit(const StoreSVFGNode *GlobalInit) {
  if (llvm::isa<UndefValue>(GlobalInit->getValue()))
    return;
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
  if (!llvm::isa<llvm::MemCpyInst>(StoreVal))
    return nullptr;
  LLVM_DEBUG(llvm::dbgs() << "Memcpy: " << *StoreVal << "\n");
  const PAGNode *DstNode = StoreNode->getPAGDstNode();
  NodeID DstID = StoreNode->getPAGDstNodeID();
  if (DstNode->getNodeKind() == PAGNode::DummyValNode || DstNode->getNodeKind() == PAGNode::DummyObjNode)
    return nullptr;
  const auto *BaseVal = DstNode->getValue()->stripPointerCasts();
  if (!llvm::isa<llvm::PointerType>(BaseVal->getType()))
    return nullptr;
  const auto *PtrTy = llvm::cast<llvm::PointerType>(BaseVal->getType());
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

  OS << "Print padding arrays\n";
  for (const auto *PaddingVar : PaddingFieldSet) {
    OS << "  - PaddingVar: " << PaddingVar->toString() << "\n";
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