#include "UseDefAnalysis/UseDefChain.h"

using namespace SVF;

void UseDefChain::insert(const LoadSVFGNode *Use, const StoreSVFGNode *Def) {
  UseDef[Use].insert(Def);
}

void UseDefChain::print(llvm::raw_ostream &OS) {
  OS << "UseDefChain::print()\n";
  for (const auto &it : UseDef) {
    const LoadSVFGNode *Use = it.first;
    OS << "- USE:" << *Use->getValue() << "\n";
    for (const auto *Def : it.second) {
      OS << "  - DEF:" << *Def->getValue() << "\n";
    }
  }
}