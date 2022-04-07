//===-- UseDefChain.h - Use-Def Chain definition ----------------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//
///
/// This file contains the declaration of the UseDefChain class,
/// which consists of an Use and all reachable Definitions.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_SVF_USEDEFANALYSIS_USEDEFCHAIN_H
#define LLVM_ANALYSIS_SVF_USEDEFANALYSIS_USEDEFCHAIN_H

#include "Graphs/SVFG.h"
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace SVF {

class UseDefChain {
  using UseDefMap = std::unordered_map<const LoadSVFGNode *, std::unordered_set<const StoreSVFGNode *>>;
  using iterator = UseDefMap::iterator;
  using const_iterator = UseDefMap::const_iterator;

  using StoreSVFGNodeList = std::unordered_set<const StoreSVFGNode *>;

public:
  /// Constructor
  UseDefChain() {}

  /// Destructor
  ~UseDefChain() {}

  /// Insert Use-Def
  void insert(const LoadSVFGNode *Use, const StoreSVFGNode *Def);

  /// Insert Def to StoerList
  void insert(const StoreSVFGNode *Def);

  /// Print Use-Def
  void print(llvm::raw_ostream &OS) const;

  /// Get DefList
  StoreSVFGNodeList &getDefList() {
    return DefList;
  }

  /// Return the begin iterator to enable range-based loop.
  iterator begin();
  const_iterator begin() const;

  /// Return the end iterator to enable range-based loop.
  iterator end();
  const_iterator end() const;

private:
  UseDefMap UseDef;
  StoreSVFGNodeList DefList;
};

} // namespace SVF

#endif