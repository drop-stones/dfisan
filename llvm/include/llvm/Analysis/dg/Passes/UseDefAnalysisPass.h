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

namespace dg {
class LLVMDependenceGraph;
} // namespace dg

namespace llvm {

struct UseDefAnalysisResult {
  // std::unique_ptr<dg::LLVMDependenceGraph> DG;
  dg::LLVMDependenceGraph *DG;

  UseDefAnalysisResult(dg::LLVMDependenceGraph *DG) : DG(DG) {}
  // UseDefAnalysisResult(std::unique_ptr<dg::LLVMDependenceGraph> DG) : DG(std::move(DG)) {}
};

class UseDefAnalysisPass : public AnalysisInfoMixin<UseDefAnalysisPass> {
  friend AnalysisInfoMixin<UseDefAnalysisPass>;

  static AnalysisKey Key; 

public:
  using Result = UseDefAnalysisResult;
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