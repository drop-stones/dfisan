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

using namespace SVF;

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
      llvm::outs() << "Base: " << *Base << "\n";
    }
  } else if (const auto *GlobalVal = llvm::dyn_cast<const GlobalValue>(FieldVal)) {
    Base = GlobalVal;
    llvm::outs() << "GlobalValue: " << *GlobalVal << "\n";
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
    //llvm::outs() << "Base StructType: " << *BaseTy << "\n";
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
  ///*
  bool Ret = false;
  if (const StructType *StructTy = llvm::dyn_cast<const StructType>(BaseTy)) {
    Ret = calculateStructOffset(OffsetVec, StructTy, RemainOffset, Base);
  } else if (const ArrayType *ArrayTy = llvm::dyn_cast<const ArrayType>(BaseTy)) {
    ArrayOffset *Offset = new ArrayOffset {ArrayTy, Base, 0};
    OffsetVec.push_back(Offset);
    Ret = true;
  }
  return Ret;
  //*/

  /*
  unsigned EleOffset = 0;
  if (const StructType *StructTy = llvm::dyn_cast<const StructType>(BaseTy)) {
    for (auto *EleTy : StructTy->elements()) {
      if (const auto *EleStructTy = llvm::dyn_cast<const StructType>(EleTy)) {
        // Dive into element struct.
        StructOffset *Offset = new StructOffset {StructTy, Base, EleOffset};
        OffsetVec.push_back(Offset);
        bool IsEnd = calculateOffsetVec(OffsetVec, EleStructTy, RemainOffset, Base);
        if (IsEnd)
          return true;
        OffsetVec.pop_back(); // Back to the parent struct
      } else {
        if (RemainOffset == 0) {
          StructOffset *Offset = new StructOffset {StructTy, Base, EleOffset};
          OffsetVec.push_back(Offset);
          return true;
        }
        RemainOffset--;
      }
      EleOffset++;
    }
  }
  return false;
  */
}


// Process one element in StructTy or ArrayTy.
bool calculateOffsetVecFromElement(FieldOffsetVector &OffsetVec, const Type *ParentTy, Type *EleTy, unsigned EleOffset, unsigned &RemainOffset, const Value *Base) {
  if (const auto *EleStructTy = llvm::dyn_cast<const StructType>(EleTy)) {
    // Dive into element struct.
    StructOffset *Offset = new StructOffset {EleStructTy, Base, EleOffset};
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
      if (const auto *ParentStructTy = llvm::dyn_cast<const StructType>(ParentTy)) {
        StructOffset *Offset = new StructOffset {ParentStructTy, Base, EleOffset};
        OffsetVec.push_back(Offset);
        llvm::outs() << "push_back " << *Offset << "\n";
      //} else if (const auto *ParentArrayTy = llvm::dyn_cast<const ArrayType>(ParentTy)) {
      //  ArrayOffset *Offset = new ArrayOffset {ParentArrayTy, Base, EleOffset};
      //  OffsetVec.push_back(Offset);
      //  llvm::outs() << "push_back " << *Offset << "\n";
      }
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
  Type *EleTy = ArrayTy->getElementType();
  //llvm::outs() << "ArrayTy->getElementType(): " << *EleTy << "\n";
  for (unsigned EleOffset = 0; EleOffset < ArrayTy->getNumElements(); EleOffset++) {
    // Dive into the element.
    ArrayOffset *Offset = new ArrayOffset {ArrayTy, Base, EleOffset};
    OffsetVec.push_back(Offset);
    bool IsEnd = calculateOffsetVecFromElement(OffsetVec, ArrayTy, EleTy, EleOffset, RemainOffset, Base);
    if (IsEnd == true)
      return true;
    OffsetVec.pop_back();
  }
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
  llvm::outs() << __func__ << ":" << Def->toString() << "\n";
  SVFIR *Pag = Svfg->getPAG();

  // Get the destination node of memcpy.
  const SVFGNode *DstNode = Svfg->getDefSVFGNode(Def->getPAGDstNode());
  if (const auto *GepNode = SVFUtil::dyn_cast<const GepSVFGNode>(DstNode)) {  // If field element
    const auto DefVars = GepNode->getDefSVFVars();
    for (const auto Field : DefVars) {
      // Get the accumulate offset.
      LocationSet Ls = Pag->getLocationSetFromBaseNode(Field);
      auto FieldIdx = Ls.accumulateConstantFieldIdx();
      llvm::outs() << "FieldIdx: " << FieldIdx << "\n";

      // Get the destination field node of memcpy.
      const SVFVar *FieldVar = Pag->getGNode(Field);
      //llvm::outs() << "FieldVar: " << FieldVar->toString() << "\n";
      const Value *FieldVal = FieldVar->getValue();
      const Value *Base = getBase(FieldVal);
      const Type *BaseTy = getTypeFromBase(Base);
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

void UseDefChain::idToUseDef() {
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
}

void UseDefChain::setDefID(const StoreSVFGNode *Def) {
  static DefID CurrDefID = 1;

  assert(Def != nullptr);
  assert(ID != 0);

  if (DefToID[Def] == 0) {
    DefToID[Def] = CurrDefID++;
  }

  if (CurrDefID == std::numeric_limits<uint16_t>::max()) {
    llvm::errs() << "DefID overflow occured.\n";
    CurrDefID = 1;
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