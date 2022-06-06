//===-- UseDefAnalysisPass.cpp - Use-Def Analysis Pass implementation -----===//
//
//===----------------------------------------------------------------------===//
///
/// This file contains the implementation of the UseDefAnalysisPass class,
/// which run Use-Def analysis and save the results to ModuleAnalysisManager.
///
//===----------------------------------------------------------------------===//

#include "UseDefAnalysis/UseDefAnalysisPass.h"
#include "UseDefAnalysis/UseDefAnalysis.h"

using namespace llvm;
using namespace SVF;

// Provide an explicit template instantiation for the static ID.
AnalysisKey UseDefAnalysisPass::Key;

UseDefAnalysisPass::Result
UseDefAnalysisPass::run(Module &M, ModuleAnalysisManager &MAM) {
  SVFModule *SvfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(M);
  SvfModule->buildSymbolTableInfo();

  UseDefAnalysis *Analyzer = new UseDefAnalysis;
  Analyzer->analyze(SvfModule);

  UseDefAnalysisResult Result{Analyzer->getSVFG(), Analyzer->getUseDef()};
  return Result;
}

PreservedAnalyses UseDefPrinterPass::run(Module &M, ModuleAnalysisManager &MAM) {
  OS << "UseDefPrinterPass::print " << M.getName() << "\n";
  auto Result = MAM.getResult<UseDefAnalysisPass>(M);
  StringRef ModuleName = M.getModuleIdentifier();
  ModuleName = ModuleName.drop_back(3);
  Result.Svfg->dump(ModuleName.str() + "-usedef-svfg");
  Result.UseDef->print(OS);
  return PreservedAnalyses::all();
}