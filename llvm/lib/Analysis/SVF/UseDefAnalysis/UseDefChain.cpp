//===-- UseDefChain.cpp - Use-Def Chain implementation --------------------===//
//
//===----------------------------------------------------------------------===//
///
/// This file contains the implementation of the UseDefChain class,
/// which consists of an Use and all reachable Definitions.
///
//===----------------------------------------------------------------------===//

#include "UseDefAnalysis/UseDefChain.h"

using namespace SVF;

void UseDefChain::insert(const LoadSVFGNode *Use, const StoreSVFGNode *Def) {
  UseDef[Use].insert(Def);
}

void UseDefChain::print(llvm::raw_ostream &OS) const {
  OS << "UseDefChain::print()\n";
  for (const auto &Iter : UseDef) {
    const LoadSVFGNode *Use = Iter.first;
    OS << "- USE:" << *Use->getValue() << "\n";
    for (const auto *Def : Iter.second) {
      OS << "  - DEF:" << *Def->getValue() << "\n";
    }
  }
}