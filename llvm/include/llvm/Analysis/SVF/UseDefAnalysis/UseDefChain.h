#ifndef LLVM_ANALYSIS_SVF_USEDEFANALYSIS_USEDEFCHAIN_H
#define LLVM_ANALYSIS_SVF_USEDEFANALYSIS_USEDEFCHAIN_H

#include "Graphs/SVFG.h"
#include <unordered_set>
#include <unordered_map>

namespace SVF {

class UseDefChain {
  using UseDefMap = std::unordered_map<const LoadSVFGNode *, std::unordered_set<const StoreSVFGNode *>>;

public:
  /// Constructor
  UseDefChain() {}

  /// Destructor
  ~UseDefChain() {}

  /// Insert Use-Def
  void insert(const LoadSVFGNode *Use, const StoreSVFGNode *Def);

  /// Print Use-Def
  void print(llvm::raw_ostream &OS);

private:
  UseDefMap UseDef;
};

} // namespace SVF

#endif