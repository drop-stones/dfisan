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

#include "UseDefAnalysis/UseDefLogger.h"
#include "UseDefAnalysis/UseDefUtils.h"

namespace SVF {

struct FieldOffset {
  enum FieldOffsetKind {
    StructKind,
    ArrayKind,
    PointerKind,
  };

  const llvm::Type  *BaseTy;
  const llvm::Value *Base;
  unsigned Offset;

private:
  const FieldOffsetKind Kind;

protected:
  FieldOffset(FieldOffsetKind Kind, const llvm::Type *BaseTy, const llvm::Value *Base, unsigned Offset) : BaseTy(BaseTy), Base(Base), Offset(Offset), Kind(Kind) {}
  FieldOffset(FieldOffsetKind Kind, const llvm::Value *Base, unsigned Offset) : FieldOffset(Kind, Base->getType(), Base, Offset) {}

public:
  FieldOffsetKind getKind() const { return Kind; }
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const FieldOffset &Offset);

struct StructOffset : FieldOffset {
  const llvm::StructType *StructTy;

  StructOffset(const llvm::StructType *StructTy, const llvm::Value *Base, unsigned Offset)
    : FieldOffset(StructKind, Base, Offset), StructTy(StructTy) {}
  
  static bool classof(const FieldOffset *F) {
    return F->getKind() == StructKind;
  }
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const StructOffset &Offset);

struct ArrayOffset : FieldOffset {
  const llvm::ArrayType *ArrayTy;

  ArrayOffset(const llvm::ArrayType *ArrayTy, const llvm::Value *Base, unsigned Offset)
    : FieldOffset(ArrayKind, Base, Offset), ArrayTy(ArrayTy) {}

  static bool classof(const FieldOffset *F) {
    return F->getKind() == ArrayKind;
  }
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const ArrayOffset &Offset);

struct PointerOffset : FieldOffset {
  const llvm::Value *Length;

  PointerOffset(const llvm::Value *Base, const llvm::Value *Length)
    : FieldOffset(PointerKind, Base, 0), Length(Length) {}
  
  static bool classof(const FieldOffset *F) {
    return F->getKind() == PointerKind;
  }
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const PointerOffset &Offset);

using FieldOffsetVector = std::vector<FieldOffset *>;

using DefID = uint16_t;
struct DefInfo {
  DefID ID;
  bool IsInstrumented;

  DefInfo(DefID ID) : ID(ID), IsInstrumented(false) {}
  DefInfo() : ID(0), IsInstrumented(false) {}
};
using DefInfoMap = std::unordered_map<const StoreSVFGNode *, DefInfo>;


class UseDefChain {
  using DefSet = std::unordered_set<const StoreSVFGNode *>;
  using UseDefMap = std::unordered_map<const LoadSVFGNode *, DefSet>;
  using iterator = UseDefMap::iterator;
  using const_iterator = UseDefMap::const_iterator;

  using FieldStoreToOffsetMap = std::unordered_map<const StoreSVFGNode *, FieldOffsetVector>;
  using SVFVarSet = std::unordered_set<const SVFVar *>;

public:
  /// Constructor
  UseDefChain(std::string ModuleName) : Logger(ModuleName) {}

  /// Destructor
  ~UseDefChain() {}

  /// Insert Use-Def
  void insert(const LoadSVFGNode *Use, const StoreSVFGNode *Def);

  /// Insert Def to StoreList
  void insertDefUsingPtr(const StoreSVFGNode *Def);

  /// Insert Field-Store Nodes
  void insertFieldStore(const SVFG *Svfg, const StoreSVFGNode *Def);

  /// Insert Def to GlobalInitList
  void insertGlobalInit(const StoreSVFGNode *Def);

  /// ID to Defs
  void idToUseDef(SVFIR *Pag);

  /// Get DefID
  DefID getDefID(const StoreSVFGNode *Def) const {
    return DefToInfo.at(Def).ID;
  }

  /// Get DefToInfo
  DefInfoMap &getDefInfoMap() {
    return DefToInfo;
  }

  /// Set to IsInstrumented.
  void setInstrumented(const StoreSVFGNode *Def) {
    DefToInfo.at(Def).IsInstrumented = true;
  }

  /// Check whether the store is instrumented or not.
  bool isInstrumented(const StoreSVFGNode *Def) const {
    return DefToInfo.at(Def).IsInstrumented;
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

  /// Get OffsetVec
  const FieldOffsetVector &getOffsetVector(const StoreSVFGNode *MemcpyNode) const {
    assert(containsOffsetVector(MemcpyNode));
    return FieldStoreMap.at(MemcpyNode);
  }

  /// Check whether the OffsetVector exists.
  bool containsOffsetVector(const StoreSVFGNode *StoreNode) const {
    auto Search = FieldStoreMap.find(StoreNode);
    return Search != FieldStoreMap.end();
  }

  /// Check whether the SVFVar is padding array or not.
  bool isPaddingArray(const SVFVar *Var) {
    return PaddingFieldSet.count(Var) != 0;
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
  DefInfoMap DefToInfo;
  FieldStoreToOffsetMap FieldStoreMap;   // Field-Store to FieldOffset
  SVFVarSet PaddingFieldSet;
  UseDefLogger Logger;

  void setDefID(const StoreSVFGNode *Def);

  const SVFVar *getArrayBaseFromMemcpy(SVFIR *Pag, const StoreSVFGNode *StoreNode);
  void mergeMemcpyIDs(SVFIR *Pag);
};

} // namespace SVF

#endif