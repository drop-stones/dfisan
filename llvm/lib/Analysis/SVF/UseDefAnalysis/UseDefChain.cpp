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
const StructType *getStructTypeFromFieldValue(const Value *FieldVal) {
  if (FieldVal == nullptr)
    return nullptr;

  // The destination must be `bitcast %struct`.
  if (const auto *Bitcast = llvm::dyn_cast<const BitCastInst>(FieldVal)) {
    const Value *BaseVal = Bitcast->getOperand(0);
    llvm::outs() << "BaseVal: " << *BaseVal << "\n";
    // The RHS must be `alloca`.
    if (const auto *Alloca = llvm::dyn_cast<const AllocaInst>(BaseVal)) {
      const Type *BaseTy = Alloca->getAllocatedType();
      llvm::outs() << "BaseTy: " << *BaseTy << "\n";
      if (const auto *StructTy = llvm::dyn_cast<const StructType>(BaseTy)) {
        return StructTy;
      }
    }
  }
  return nullptr;
}

// Return true if offset calculation is ended.
bool calculateOffsetVec(std::vector<unsigned> &OffsetVec, const StructType *StructTy, unsigned &RemainOffset) {
  unsigned EleOffset = 0;
  for (auto *EleTy : StructTy->elements()) {
    if (const auto *EleStructTy = llvm::dyn_cast<const StructType>(EleTy)) {
      // Dive into element struct.
      OffsetVec.push_back(EleOffset);
      bool IsEnd = calculateOffsetVec(OffsetVec, EleStructTy, RemainOffset);
      if (IsEnd)
        return true;
      OffsetVec.pop_back(); // Back to the parent struct
    } else {
      if (RemainOffset == 0) {
        OffsetVec.push_back(EleOffset);
        return true;
      }
      RemainOffset--;
    }
    EleOffset++;
  }
  return false;
}
} // anonymous namespace

void UseDefChain::insert(const LoadSVFGNode *Use, const StoreSVFGNode *Def) {
  UseDef[Use].insert(Def);
}

void UseDefChain::insertDefUsingPtr(const StoreSVFGNode *Def) {
  DefUsingPtrList.insert(Def);
}

void UseDefChain::insertMemcpy(const SVFG *Svfg, const StoreSVFGNode *Def) {
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
      llvm::outs() << "FieldVar: " << FieldVar->toString() << "\n";
      const Value *FieldVal = FieldVar->getValue();
      const StructType *StructTy = getStructTypeFromFieldValue(FieldVal);
      if (StructTy == nullptr)
        continue;
      
      std::vector<unsigned> OffsetVec;
      unsigned RemainOffset = FieldIdx;
      calculateOffsetVec(OffsetVec, StructTy, RemainOffset);
      llvm::outs() << "OffsetVec: (";
      for (auto Offset : OffsetVec)
        llvm::outs() << Offset << ", ";
      llvm::outs() << ")\n";
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