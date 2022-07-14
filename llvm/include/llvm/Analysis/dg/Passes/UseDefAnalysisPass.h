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
#include "dg/Passes/UseDefBuilder.h"

namespace llvm {

class UseDefAnalysisPass : public AnalysisInfoMixin<UseDefAnalysisPass> {
  friend AnalysisInfoMixin<UseDefAnalysisPass>;

  static AnalysisKey Key; 

public:
  class Result {
    std::unique_ptr<dg::UseDefBuilder> Builder{nullptr};

  public:
    Result(std::unique_ptr<dg::UseDefBuilder> &&Builder) : Builder(std::move(Builder)) {}

    dg::UseDefBuilder *getBuilder() { return Builder.get(); }
    dg::LLVMDependenceGraph *getDG() { return Builder->getDG(); }
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