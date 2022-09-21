//===-- UseDefAnalysisPass.cpp - Use-Def Analysis Pass implementation -----===//
//
//===----------------------------------------------------------------------===//
///
/// This file contains the implementation of the UseDefAnalysisPass class,
/// which run Use-Def analysis and save the results to ModuleAnalysisManager.
///
//===----------------------------------------------------------------------===//

#include "dg/Passes/UseDefAnalysisPass.h"
#include "dg/Passes/ProtectionTargetAnalysisPass.h"
#include "dg/llvm/LLVMDependenceGraph.h"
#include "dg/llvm/LLVMDependenceGraphBuilder.h"
#include "dg/llvm/LLVMSlicer.h"
#include "dg/llvm/LLVMDG2Dot.h"

#include "dg/Passes/UseDefBuilder.h"
#include "dg/Passes/UseDefLogger.h"
#include "dg/Passes/DfiUtils.h"

using namespace llvm;
using namespace dg;

// Provide an explicit template instantiation for the static ID.
AnalysisKey UseDefAnalysisPass::Key;

UseDefAnalysisPass::Result
UseDefAnalysisPass::run(Module &M, ModuleAnalysisManager &MAM) {
  /// Error at assert(entryNode) in dda::LLVMDefUseAnalysis::addDataDependencies().
  // llvmdg::LLVMDependenceGraphOptions Opts;
  // Opts.PTAOptions.analysisType = dg::LLVMPointerAnalysisOptions::AnalysisType::svf;
  // std::unique_ptr<dg::UseDefBuilder> Builder = std::make_unique<dg::UseDefBuilder>(&M, Opts);

  auto &AnalysisResult = MAM.getResult<ProtectionTargetAnalysisPass>(M);
  AnalysisResult.dump(llvm::outs());
  assert(!AnalysisResult.beforeReplacement() && "ReplaceWithSafeAlloc is not done yet");

  std::unique_ptr<dg::UseDefBuilder> Builder = std::make_unique<dg::UseDefBuilder>(&M);
/*
  Builder->buildDG();
  Builder->assignDefIDs();
  Builder->printProtectInfo(llvm::outs());

  debug::UseDefLogger Logger{M};
  Logger.logDefInfo(Builder.get());
*/

  UseDefAnalysisPass::Result Result { std::move(Builder) };

  return Result;
}

PreservedAnalyses
UseDefPrinterPass::run(Module &M, ModuleAnalysisManager &MAM) {
  OS << "UseDefPrinterPass::print " << M.getName() << "\n";
  auto &Result = MAM.getResult<UseDefAnalysisPass>(M);
  auto *Builder = Result.getBuilder();

  debug::LLVMDG2Dot Dumper(Builder->getDG());
  Dumper.dump("dg.dot");

  Builder->printUseDef(OS);
  // Builder->printDefInfoMap(OS);
  // Builder->dump(OS);

  return PreservedAnalyses::all();
}
