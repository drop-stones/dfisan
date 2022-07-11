//===-- UseDefAnalysisPass.h - Use-Def Analysis Pass definition -*- C++ -*-===//
//
//===----------------------------------------------------------------------===//
///
/// This file contains the declaration of the UseDefAnalysisPass class,
/// which run Use-Def analysis and save the results to ModuleAnalysisManager.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_DG_PASSES_USEDEFANALYSIS_PASS_H
#define LLVM_ANALYSIS_DG_PASSES_USEDEFANALYSIS_PASS_H

#include "llvm/IR/PassManager.h"
#include "dg/llvm/LLVMDependenceGraph.h"
#include "dg/llvm/LLVMDependenceGraphBuilder.h"

namespace llvm {

class UseDefAnalysisPass : public AnalysisInfoMixin<UseDefAnalysisPass> {
  friend AnalysisInfoMixin<UseDefAnalysisPass>;

  static AnalysisKey Key; 

public:
  struct Result {
    std::unique_ptr<dg::llvmdg::LLVMDependenceGraphBuilder> Builder{nullptr};
    std::unique_ptr<dg::LLVMDependenceGraph> DG{nullptr};

    Result(std::unique_ptr<dg::llvmdg::LLVMDependenceGraphBuilder> &&Builder, std::unique_ptr<dg::LLVMDependenceGraph> &&DG)
      : Builder(std::move(Builder)), DG(std::move(DG)) {}
    dg::llvmdg::LLVMDependenceGraphBuilder &getBuilder() { return *Builder.get(); }
    dg::LLVMDependenceGraph &getDG() { return *DG.get(); }
    // bool invalidate();
  };
  
  Result run(Module &M, ModuleAnalysisManager &MAM);
};

class UseDefPrinterPass : public PassInfoMixin<UseDefPrinterPass> {
  raw_ostream &OS;

public:
  explicit UseDefPrinterPass(raw_ostream &OS) : OS(OS) {}
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
};

} // namespace llvm

#endif