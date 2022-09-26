
#ifndef LLVM_ANALYSIS_DG_PASSES_USEDEFBUILDER_H
#define LLVM_ANALYSIS_DG_PASSES_USEDEFBUILDER_H

#include "dg/llvm/LLVMDependenceGraph.h"
#include "dg/llvm/LLVMDependenceGraphBuilder.h"
#include "dg/Passes/DfiProtectInfo.h"
#include "dg/Passes/UseDefIterator.h"
#include "dg/Passes/DfiLLVMDependenceGraphBuilder.h"

namespace dg {

class UseDefBuilder {
public:
  UseDefBuilder(llvm::Module *M, ValueSet &Aligned, ValueSet &Unaligned)
    : UseDefBuilder(M, Aligned, Unaligned, {}) {}
  UseDefBuilder(llvm::Module *M, ValueSet &Aligned, ValueSet &Unaligned, const llvmdg::LLVMDependenceGraphOptions &Opts)
    : ProtectInfo(std::make_unique<DfiProtectInfo>(Aligned, Unaligned)), DgBuilder(new llvmdg::DfiLLVMDependenceGraphBuilder(ProtectInfo.get(), M, Opts)),  M(M) {}
  
  LLVMDependenceGraph *getDG() { return DG.get(); }
  LLVMDataDependenceAnalysis *getDDA() { return DG->getDDA(); }
  LLVMPointerAnalysis *getPTA() { return (getDG() != nullptr) ? DG->getPTA() : DgBuilder->getPTA(); }
  std::unique_ptr<LLVMDependenceGraph> &&moveDG() { return std::move(DG); }

  DfiProtectInfo *getProtectInfo() { return ProtectInfo.get(); }
  std::unique_ptr<DfiProtectInfo> &&moveProtectInfo() { return std::move(ProtectInfo); }

  LLVMDependenceGraph *buildDG() {
    DG = DgBuilder->build();
    return DG.get();
  }

  const std::vector<dda::RWNode *> &getGlobals() {
    return getDDA()->getGlobals();
  }

  bool isDef(llvm::Value *Def);
  bool isUse(llvm::Value *Use);

  void printUseDef(llvm::raw_ostream &OS);
  void dump(llvm::raw_ostream &OS);

private:
  std::unique_ptr<DfiProtectInfo> ProtectInfo{nullptr};
  std::unique_ptr<llvmdg::DfiLLVMDependenceGraphBuilder> DgBuilder{nullptr};
  std::unique_ptr<LLVMDependenceGraph> DG{nullptr};
  llvm::Module *M;

public:
  LLVMNodeIterator def_begin() { return LLVMNodeIterator::begin(*this, &UseDefBuilder::isDef, getConstructedFunctions()); }
  LLVMNodeIterator def_end()   { return LLVMNodeIterator::end(*this, &UseDefBuilder::isDef, getConstructedFunctions()); }
  LLVMNodeIterator use_begin() { return LLVMNodeIterator::begin(*this, &UseDefBuilder::isUse, getConstructedFunctions()); }
  LLVMNodeIterator use_end()   { return LLVMNodeIterator::end(*this, &UseDefBuilder::isUse, getConstructedFunctions()); }
  GlobalInitIterator glob_begin() { return GlobalInitIterator::begin(getGlobals()); }
  GlobalInitIterator glob_end()   { return GlobalInitIterator::end(getGlobals()); }
};

} // namespace dg

#endif