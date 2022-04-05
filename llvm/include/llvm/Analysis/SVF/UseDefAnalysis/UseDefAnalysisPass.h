//===-- UseDefAnalysisPass.h - Use-Def Analysis Pass definition -*- C++ -*-===//
//
//===----------------------------------------------------------------------===//
///
/// This file contains the declaration of the UseDefAnalysisPass class,
/// which run Use-Def analysis and save the results to ModuleAnalysisManager.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_SVF_USEDEFANALYSIS_USEDEFANALYSIS_PASS_H
#define LLVM_ANALYSIS_SVF_USEDEFANALYSIS_USEDEFANALYSIS_PASS_H

#include "llvm/IR/PassManager.h"

namespace SVF {
class SVFG;
class UseDefChain;
} // namespace SVF

namespace llvm {

struct UseDefAnalysisResult {
  SVF::SVFG *Svfg;
  SVF::UseDefChain *UseDef;

  UseDefAnalysisResult(SVF::SVFG *Svfg, SVF::UseDefChain *UseDef)
    : Svfg(Svfg), UseDef(UseDef) {}
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