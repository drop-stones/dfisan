#include "dg/Passes/UseDefBuilder.h"

using namespace dg;

bool
UseDefBuilder::isDef(llvm::Value *Val) {
  return getDDA()->isDef(Val);
}
bool
UseDefBuilder::isUse(llvm::Value *Val) {
  return getDDA()->isUse(Val);
}

void
UseDefBuilder::assignDefIDs() {
  for (const auto &Iter : *DG) {
    auto *Val = Iter.first;
    if (isDef(Val)) {
      assignDefID(Val);
    }
  }
}

void
UseDefBuilder::assignDefID(llvm::Value *Def) {
  static DefID CurrID = 1;
  if (DefToInfo.count(Def) == 0) {
    DefToInfo.emplace(Def, CurrID);

    if (CurrID == USHRT_MAX) {
      llvm::errs() << "DefID overflow!!\n";
      CurrID = 1;
    } else {
      CurrID++;
    }
  }
}

DefID
UseDefBuilder::getDefID(llvm::Value *Key) {
  assert(DefToInfo.count(Key) != 0);
  return DefToInfo[Key].ID;
}

void
UseDefBuilder::printUseDef(llvm::raw_ostream &OS) {
  OS << __func__ << "\n";
  for (const auto &Iter : *DG) {
    auto *Val = Iter.first;
    assert(Val != nullptr);

    if (isUse(Val)) {
      OS << "Use: " << *Val << "\n";
      for (auto *Def : getDDA()->getLLVMDefinitions(Val)) {
        OS << " - DefID[" << DefToInfo[Def].ID << "]: " << *Def << "\n";
      }
    }
  }
}

void
UseDefBuilder::printDefInfoMap(llvm::raw_ostream &OS) {
  OS << __func__ << "\n";
  for (const auto &Iter : DefToInfo) {
    auto *Val = Iter.first;
    auto &DefInfo = Iter.second;

    OS << "DefID[" << DefInfo.ID << "]: " << *Val << "\n";
  }
}
