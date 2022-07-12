
#ifndef LLVM_ANALYSIS_DG_PASSES_USEDEFBUILDER_H
#define LLVM_ANALYSIS_DG_PASSES_USEDEFBUILDER_H

#include "dg/llvm/LLVMDependenceGraph.h"
#include "dg/llvm/LLVMDependenceGraphBuilder.h"

namespace dg {

using DefID = uint16_t;
struct DefInfo {
  DefID ID;
  bool IsInstrumented;

  DefInfo(DefID ID) : ID(ID), IsInstrumented(false) {}
  DefInfo() : DefInfo(0) {}
};
using DefInfoMap = std::unordered_map<llvm::Value *, DefInfo>;

class UseDefBuilder : public llvmdg::LLVMDependenceGraphBuilder {
public:
  struct def_iterator {
    using iterator = LLVMDependenceGraph::iterator;
    using reference = iterator::reference;
    using pointer = iterator::pointer;
    def_iterator(UseDefBuilder &B, iterator Begin, iterator End) : Builder(B), Iter(Begin), End(End) {
      while(!Builder.isDef(Iter->first)) {
        Iter++;
        if (Iter == End)
          break;
      }
    }
    reference operator*() const { return *Iter; }
    iterator operator->() { return Iter; }
    def_iterator &operator++();
    def_iterator operator++(int);
    friend bool operator==(const def_iterator &a, const def_iterator &b) { return a.Iter == b.Iter; }
    friend bool operator!=(const def_iterator &a, const def_iterator &b) { return a.Iter != b.Iter; }
  private:
    UseDefBuilder &Builder;
    iterator Iter;
    const iterator End;
  };
  struct use_iterator {
    using iterator = LLVMDependenceGraph::iterator;
    using reference = iterator::reference;
    using pointer = iterator::pointer;
    use_iterator(UseDefBuilder &B, iterator Begin, iterator End) : Builder(B), Iter(Begin), End(End) {
      while(!Builder.isUse(Iter->first)) {
        Iter++;
        if (Iter == End)
          break;
      }
    }
    reference operator*() const { return *Iter; }
    iterator operator->() { return Iter; }
    use_iterator &operator++();
    use_iterator operator++(int);
    friend bool operator==(const use_iterator &a, const use_iterator &b) { return a.Iter == b.Iter; }
    friend bool operator!=(const use_iterator &a, const use_iterator &b) { return a.Iter != b.Iter; }
  private:
    UseDefBuilder &Builder;
    iterator Iter;
    const iterator End;
  };

public:
  UseDefBuilder(llvm::Module *M)
    : UseDefBuilder(M, {}) {}
  UseDefBuilder(llvm::Module *M, const llvmdg::LLVMDependenceGraphOptions &Opts)
    : llvmdg::LLVMDependenceGraphBuilder(M, Opts) {}
  
  LLVMDependenceGraph *getDG() { return DG.get(); }

  LLVMDependenceGraph *buildDG() {
    DG = llvmdg::LLVMDependenceGraphBuilder::build();
    return DG.get();
  }

  bool isDef(llvm::Value *Def);
  bool isUse(llvm::Value *Use);

  void assignDefIDs();
  DefID getDefID(llvm::Value *Key);

  void printUseDef(llvm::raw_ostream &OS);
  void printDefInfoMap(llvm::raw_ostream &OS);

  def_iterator def_begin();
  def_iterator def_end();

  use_iterator use_begin();
  use_iterator use_end();

private:
  std::unique_ptr<LLVMDependenceGraph> DG{nullptr};
  DefInfoMap DefToInfo;

  void assignDefID(llvm::Value *Def);
};

} // namespace dg

#endif