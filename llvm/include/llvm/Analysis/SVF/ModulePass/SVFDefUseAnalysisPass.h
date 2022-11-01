//===-- SVFDefUseAnalysisPass.h - Def-Use Analysis Pass definition -*- C++ -*-===//
//
//===-------------------------------------------------------------------------===//
///
/// This file contains the declaration of the SVFDefUseAnalysisPass class,
/// which run Use-Def analysis and save the results to ModuleAnalysisManager.
///
//===-------------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_SVF_DFISAN_DEFUSEANALYSIS_H_
#define LLVM_ANALYSIS_SVF_DFISAN_DEFUSEANALYSIS_H_

#include "llvm/IR/PassManager.h"

namespace SVF {
class ProtectInfo;
} // namespace SVF

namespace llvm {

class SVFDefUseAnalysisPass : public AnalysisInfoMixin<SVFDefUseAnalysisPass> {
  friend AnalysisInfoMixin<SVFDefUseAnalysisPass>;

  static AnalysisKey Key;

public:
  class Result {
    SVF::ProtectInfo *ProtInfo;
  public:
    Result(SVF::ProtectInfo *ProtInfo) : ProtInfo(ProtInfo) {}

    SVF::ProtectInfo *getProtectInfo() { return ProtInfo; }
  };

  Result run(Module &M, ModuleAnalysisManager &MAM);
};

class SVFDefUsePrinterPass : public PassInfoMixin<SVFDefUsePrinterPass> {
  raw_ostream &OS;

public:
  explicit SVFDefUsePrinterPass(raw_ostream &OS) : OS(OS) {}
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
};

} // namespace llvm

#endif
