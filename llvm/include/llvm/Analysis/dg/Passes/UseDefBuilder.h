
#ifndef LLVM_ANALYSIS_DG_PASSES_USEDEFBUILDER_H
#define LLVM_ANALYSIS_DG_PASSES_USEDEFBUILDER_H

#include "dg/llvm/LLVMDependenceGraph.h"
#include "dg/llvm/LLVMDependenceGraphBuilder.h"
#include "dg/Passes/UseDefIterator.h"

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

private:
  std::unique_ptr<LLVMDependenceGraph> DG{nullptr};
  DefInfoMap DefToInfo;

  void assignDefID(llvm::Value *Def);

  // Iterator
  class DefIterator : public ConditionalIterator<LLVMDependenceGraph::iterator, UseDefBuilder, llvm::Value *> {
    using iterator = LLVMDependenceGraph::iterator;
    using cond = bool (UseDefBuilder::*)(llvm::Value *);
  public:
    DefIterator(UseDefBuilder &B, cond Cond, iterator Begin, iterator End) : ConditionalIterator(B, Cond, Begin, End) {}
  };
  class UseIterator : public ConditionalIterator<LLVMDependenceGraph::iterator, UseDefBuilder, llvm::Value *> {
    using iterator = LLVMDependenceGraph::iterator;
    using cond = bool (UseDefBuilder::*)(llvm::Value *);
  public:
    UseIterator(UseDefBuilder &B, cond Cond, iterator Begin, iterator End) : ConditionalIterator(B, Cond, Begin, End) {}
  };

public:
  DefIterator def_begin() { return DefIterator(*this, &UseDefBuilder::isDef, DG->begin(), DG->end()); }
  DefIterator def_end()   { return DefIterator(*this, &UseDefBuilder::isDef, DG->end(),   DG->end()); }
  UseIterator use_begin() { return UseIterator(*this, &UseDefBuilder::isUse, DG->begin(), DG->end()); }
  UseIterator use_end()   { return UseIterator(*this, &UseDefBuilder::isUse, DG->end(),   DG->end()); }
};

} // namespace dg

#endif