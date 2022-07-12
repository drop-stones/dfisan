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
  for (auto DI = def_begin(), DE = def_end(); DI != DE; DI++) {
    auto *Def = DI->first;
    assert(isDef(Def));
    assignDefID(Def);
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
  for (auto UI = use_begin(), UE = use_end(); UI != UE; UI++) {
    auto *Use = UI->first;
    OS << "Use: " << *Use << "\n";
    for (auto *Def : getDDA()->getLLVMDefinitions(Use)) {
      OS << " - DefID[" << getDefID(Def) << "]: " << *Def << "\n";
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
