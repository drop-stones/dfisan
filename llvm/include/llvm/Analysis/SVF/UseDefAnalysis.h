#ifndef LLVM_ANALYSIS_SVF_USE_DEF_ANALYSIS_H
#define LLVM_ANALYSIS_SVF_USE_DEF_ANALYSIS_H

#include "llvm/IR/PassManager.h"

namespace llvm {

struct UseDef {
  // TODO
};

class UseDefAnalysis : public AnalysisInfoMixin<UseDefAnalysis> {
  friend AnalysisInfoMixin<UseDefAnalysis>;

  static AnalysisKey Key;

public:
  using Result = UseDef;
  Result run(Module &M, ModuleAnalysisManager &MAM);
};

class UseDefPrinterPass : public PassInfoMixin<UseDefPrinterPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
};

} // namespace llvm

#endif