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
  using DefSet = std::unordered_set<const StoreSVFGNode *>;
  using UseDefMap = std::unordered_map<const LoadSVFGNode *, DefSet>;
  using iterator = UseDefMap::iterator;
  using const_iterator = UseDefMap::const_iterator;

  using DefID = uint16_t;
  using DefIdMap = std::unordered_map<const StoreSVFGNode *, DefID>;

public:
  /// Constructor
  UseDefChain() {}

  /// Destructor
  ~UseDefChain() {}

  /// Insert Use-Def
  void insert(const LoadSVFGNode *Use, const StoreSVFGNode *Def);

  /// Insert Def to StoreList
  void insertDefUsingPtr(const StoreSVFGNode *Def);

  /// Insert Def to GlobalInitList
  void insertGlobalInit(const StoreSVFGNode *Def);

  /// ID to Defs
  void idToUseDef();

  /// Get DefID
  DefID getDefID(const StoreSVFGNode *Def) const {
    return DefToID.at(Def);
  }

  /// Print Use-Def
  void print(llvm::raw_ostream &OS) const;

  /// Get DefUsingPtrList
  DefSet &getDefUsingPtrList() {
    return DefUsingPtrList;
  }

  /// Get GlobalInitList
  DefSet &getGlobalInitList() {
    return GlobalInitList;
  }

  /// Return the begin iterator to enable range-based loop.
  iterator begin();
  const_iterator begin() const;

  /// Return the end iterator to enable range-based loop.
  iterator end();
  const_iterator end() const;

private:
  UseDefMap UseDef;
  DefSet DefUsingPtrList;
  DefSet GlobalInitList;
  DefIdMap DefToID;

  void setDefID(const StoreSVFGNode *Def);
};

} // namespace SVF

#endif