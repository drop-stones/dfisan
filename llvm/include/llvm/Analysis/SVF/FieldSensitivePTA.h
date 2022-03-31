#ifndef LLVM_ANALYSIS_SVF_FIELD_SENSITIVE_PTA_H
#define LLVM_ANALYSIS_SVF_FIELD_SENSITIVE_PTA_H

#include "llvm/IR/PassManager.h"
#include <cassert>

namespace llvm {

class FieldSensitivePTA : public AnalysisInfoMixin<FieldSensitivePTA> {
  friend AnalysisInfoMixin<FieldSensitivePTA>;

  static AnalysisKey Key;

public:
  using Result = PreservedAnalyses;
  Result run(Module &M, ModuleAnalysisManager &MAM);
};

class FieldSensitivePTAPrinterPass : public PassInfoMixin<FieldSensitivePTAPrinterPass> {
  raw_ostream &OS;

public:
  FieldSensitivePTAPrinterPass(raw_ostream &OS) : OS(OS) {}
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
};

} // namespace llvm

#endif // LLVM_ANALYSIS_SVF_FIELD_SENSITIVE_PTA_H