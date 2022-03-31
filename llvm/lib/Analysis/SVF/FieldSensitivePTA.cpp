#include "FieldSensitivePTA.h"

using namespace llvm;

// Provide an explicit template instantiation for the static ID.
AnalysisKey FieldSensitivePTA::Key;

FieldSensitivePTA::Result
FieldSensitivePTA::run(Module &M, ModuleAnalysisManager &MAM) {
  errs() << M.getName() << "\n";
  return PreservedAnalyses::all();
}

PreservedAnalyses FieldSensitivePTAPrinterPass::run(Module &M, ModuleAnalysisManager &MAM) {
  auto result = MAM.getResult<FieldSensitivePTA>(M);
  return PreservedAnalyses::all();
}