
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
  void dump(llvm::raw_ostream &OS);

private:
  std::unique_ptr<LLVMDependenceGraph> DG{nullptr};
  DefInfoMap DefToInfo;

  void assignDefID(llvm::Value *Def);

public:
  LLVMNodeIterator def_begin() { return LLVMNodeIterator::begin(*this, &UseDefBuilder::isDef, getConstructedFunctions()); }
  LLVMNodeIterator def_end()   { return LLVMNodeIterator::end(*this, &UseDefBuilder::isDef, getConstructedFunctions()); }
  LLVMNodeIterator use_begin() { return LLVMNodeIterator::begin(*this, &UseDefBuilder::isUse, getConstructedFunctions()); }
  LLVMNodeIterator use_end()   { return LLVMNodeIterator::end(*this, &UseDefBuilder::isUse, getConstructedFunctions()); }
};

} // namespace dg

#endif