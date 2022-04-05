#include "llvm/Transforms/Instrumentation/DataFlowIntegritySanitizer.h"
#include "UseDefAnalysis/UseDefAnalysisPass.h"
#include "UseDefAnalysis/UseDefChain.h"

using namespace llvm;

PreservedAnalyses
DataFlowIntegritySanitizerPass::run(Module &M, ModuleAnalysisManager &MAM) {
  auto Result = MAM.getResult<UseDefAnalysisPass>(M);
  Result.UseDef->print(dbgs());
  return PreservedAnalyses::all();
}