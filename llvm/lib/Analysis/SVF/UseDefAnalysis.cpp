#include "UseDefAnalysis.h"
#include "SparseValueFlowGraph.h"

using namespace llvm;

// Provide an explicit template instantiation for the static ID.
AnalysisKey UseDefAnalysis::Key;

UseDefAnalysis::Result
UseDefAnalysis::run(Module &M, ModuleAnalysisManager &MAM) {
  auto SvfResult = MAM.getResult<SparseValueFlowGraph>(M);
  UseDef Result{};
  return Result;
}

PreservedAnalyses UseDefPrinterPass::run(Module &M, ModuleAnalysisManager &MAM) {
  errs() << "UseDefPrinterPass::print " << M.getName() << "\n";
  auto Result = MAM.getResult<UseDefAnalysis>(M);
  return PreservedAnalyses::all();
}