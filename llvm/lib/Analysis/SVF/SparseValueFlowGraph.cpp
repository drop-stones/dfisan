#include "SparseValueFlowGraph.h"
#include "SVF-FE/LLVMUtil.h"
#include "SVF-FE/ICFGBuilder.h"
#include "SVF-FE/SVFIRBuilder.h"
#include "WPA/Andersen.h"

using namespace llvm;
using namespace SVF;

// Provide an explicit template instantiation for the static ID.
AnalysisKey SparseValueFlowGraph::Key;

SparseValueFlowGraph::Result
SparseValueFlowGraph::run(Module &M, ModuleAnalysisManager &MAM) {
  LLVMModuleSet *Modules = LLVMModuleSet::getLLVMModuleSet();
  SVFModule *SvfModule = Modules->buildSVFModule(M);
  SvfModule->buildSymbolTableInfo();
  SVFIRBuilder Builder;
  SVFIR *Pag = Builder.build(SvfModule);
  Andersen *Pta = AndersenWaveDiff::createAndersenWaveDiff(Pag);
  SVFGBuilder SvfBuilder;
  SVFG *Svfg = SvfBuilder.buildFullSVFG(Pta);

  SparseValueFlowGraphResult Result{Pag, Pta, Svfg};
  return Result;
}

PreservedAnalyses SparseValueFlowGraphDumpPass::run(Module &M, ModuleAnalysisManager &MAM) {
  auto Result = MAM.getResult<SparseValueFlowGraph>(M);
  Result.Pag->dump("pag");
  Result.Svfg->dump("svfg");
  return PreservedAnalyses::all();
}