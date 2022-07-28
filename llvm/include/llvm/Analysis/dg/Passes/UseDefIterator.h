#ifndef LLVM_ANALYSIS_DG_PASSES_USEDEFITERATOR_H
#define LLVM_ANALYSIS_DG_PASSES_USEDEFITERATOR_H

#include "llvm/IR/Value.h"
#include "dg/llvm/LLVMDependenceGraph.h"
#include "dg/ReadWriteGraph/RWNode.h"

namespace dg {
class UseDefBuilder;
} // namespace dg

class LLVMNodeIterator {
  using LLVMDependenceGraphMapTy = const std::map<llvm::Value *, dg::LLVMDependenceGraph *>;
  using BBlocksMapTy = dg::LLVMDependenceGraph::BBlocksMapT;
  using LLVMNodeListTy = std::list<dg::LLVMNode *>;
  using CondTy = bool (dg::UseDefBuilder:: *)(llvm::Value *);

public:
  static LLVMNodeIterator begin(dg::UseDefBuilder &UseDef, CondTy Cond, LLVMDependenceGraphMapTy &CF) {
    return LLVMNodeIterator(UseDef, Cond, CF, CF.begin());
  }
  static LLVMNodeIterator end(dg::UseDefBuilder &UseDef, CondTy Cond, LLVMDependenceGraphMapTy &CF) {
    return LLVMNodeIterator(UseDef, Cond, CF, CF.end());
  }

  LLVMNodeIterator &operator++() {
    nextNode();
    return *this;
  }
  LLVMNodeIterator operator++(int) {
    auto Ret = *this;
    ++(*this);
    return Ret;
  }

  LLVMNodeListTy::iterator::reference operator*() const { return *NodeIter; }
  LLVMNodeListTy::iterator::pointer operator->() const { return &(*NodeIter); }
  friend bool operator==(const LLVMNodeIterator &Lhs, const LLVMNodeIterator &Rhs) {
    return
      (Lhs.isEndFunc() && Rhs.isEndFunc()) ||
      (Lhs.FuncIter == Rhs.FuncIter && Lhs.BBIter == Rhs.BBIter && Lhs.NodeIter == Rhs.NodeIter);
  }
  friend bool operator!=(const LLVMNodeIterator &Lhs, const LLVMNodeIterator &Rhs) {
    return !(Lhs == Rhs);
  }

private:
  LLVMNodeIterator(dg::UseDefBuilder &UseDef, CondTy Cond, LLVMDependenceGraphMapTy &CF, LLVMDependenceGraphMapTy::const_iterator FuncIter)
    : UseDef(UseDef), Cond(Cond), CF(CF), FuncIter(FuncIter) {
    if (isEndFunc()) return;
    if (getBBMap().empty())
      nextFunc();
    BBIter = getBBMap().begin();
    if (getNodeList().empty())
      nextBB();
    NodeIter = getNodeList().begin();
    if (!isValidNode())
      nextNode();
  }

  LLVMDependenceGraphMapTy &getCFMap() const { return CF; }
  BBlocksMapTy &getBBMap() const { return FuncIter->second->getBlocks(); }
  LLVMNodeListTy &getNodeList() const { return BBIter->second->getNodes(); }

  bool isEndFunc() const { return FuncIter == getCFMap().end(); }
  bool isEndBB()   const { return BBIter == getBBMap().end(); }
  bool isEndNode() const { return NodeIter == getNodeList().end(); }
  bool isValidNode() const { return (UseDef.*Cond)((*NodeIter)->getValue()); }

  /// Skip to the next Func which has BBlocks
  void nextFunc() {
    if (isEndFunc()) return;
    do {
      FuncIter++;
    } while (!isEndFunc() && getBBMap().empty());
  }
  /// Skip to the next BBlock which has some LLVMNodes
  void nextBB() {
    if (isEndFunc()) return;
    do {
      if (!isEndBB())
        BBIter++;
      if (isEndBB()) {
        nextFunc();
        if (isEndFunc()) return;
        BBIter = getBBMap().begin();
      }
      if (isEndFunc()) return;
    } while(!isEndBB() && getNodeList().empty());
  }
  /// Skip to the next LLVMNode
  void nextNode() {
    if (isEndFunc()) return;
    do {
      if (!isEndNode())
        NodeIter++;
      if (isEndNode()) {
        nextBB();
        if (isEndFunc()) return;
        NodeIter = getNodeList().begin();
      }
      if (isEndFunc()) return;
    } while(!isEndNode() && !isValidNode());
    assert(isValidNode());
  }

  dg::UseDefBuilder &UseDef;
  CondTy Cond;                    // Conditional function
  LLVMDependenceGraphMapTy &CF;   // ConstructedFunctions
  LLVMDependenceGraphMapTy::const_iterator FuncIter;
  BBlocksMapTy::iterator BBIter;
  LLVMNodeListTy::iterator NodeIter;
};

class GlobalInitIterator {
  using GlobalVecTy = const std::vector<dg::dda::RWNode *>;
  using Iterator = GlobalVecTy::const_iterator;
public:
  static GlobalInitIterator begin(GlobalVecTy &GlobalVec) {
    return GlobalInitIterator(GlobalVec, GlobalVec.begin());
  }
  static GlobalInitIterator end(GlobalVecTy &GlobalVec) {
    return GlobalInitIterator(GlobalVec, GlobalVec.end());
  }

  GlobalInitIterator &operator++() {
    do {
      Iter++;
    } while(!isEnd() && !isGlobalInit());
    return *this;
  }
  GlobalInitIterator operator++(int) {
    auto Ret = *this;
    ++(*this);
    return Ret;
  }

  Iterator::reference operator*() const { return *Iter; }
  Iterator::pointer operator->() const { return &(*Iter); }
  friend bool operator==(const GlobalInitIterator &Lhs, const GlobalInitIterator &Rhs) { return Lhs.Iter == Rhs.Iter; }
  friend bool operator!=(const GlobalInitIterator &Lhs, const GlobalInitIterator &Rhs) { return !(Lhs == Rhs); }

private:
  GlobalInitIterator(GlobalVecTy &GlobalVec, Iterator Iter) : GlobalVec(GlobalVec), Iter(Iter) {}

  bool isEnd() const {
    return Iter == GlobalVec.end();
  }
  bool isGlobalInit() const {
    return (*Iter)->isGlobal() && (*Iter)->isDef();
  }

  GlobalVecTy &GlobalVec;
  Iterator Iter;
};

#endif