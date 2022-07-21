//===-- UseDefAnalysisPass.cpp - Use-Def Analysis Pass implementation -----===//
//
//===----------------------------------------------------------------------===//
///
/// This file contains the implementation of the UseDefAnalysisPass class,
/// which run Use-Def analysis and save the results to ModuleAnalysisManager.
///
//===----------------------------------------------------------------------===//

#include "dg/Passes/UseDefAnalysisPass.h"
#include "dg/llvm/LLVMDependenceGraph.h"
#include "dg/llvm/LLVMDependenceGraphBuilder.h"
#include "dg/llvm/LLVMSlicer.h"
#include "dg/llvm/LLVMDG2Dot.h"

#include "dg/Passes/UseDefBuilder.h"
#include "dg/Passes/UseDefLogger.h"

using namespace llvm;
using namespace dg;

// Provide an explicit template instantiation for the static ID.
AnalysisKey UseDefAnalysisPass::Key;

UseDefAnalysisPass::Result
UseDefAnalysisPass::run(Module &M, ModuleAnalysisManager &MAM) {
  std::unique_ptr<dg::UseDefBuilder> Builder = std::make_unique<dg::UseDefBuilder>(&M);
  Builder->buildDG();
  Builder->assignDefIDs();

  debug::UseDefLogger Logger{M};
  Logger.logDefInfo(Builder.get());

  UseDefAnalysisPass::Result Result { std::move(Builder) };

  return Result;
}

PreservedAnalyses
UseDefPrinterPass::run(Module &M, ModuleAnalysisManager &MAM) {
  OS << "UseDefPrinterPass::print " << M.getName() << "\n";
  auto &Result = MAM.getResult<UseDefAnalysisPass>(M);
  auto *Builder = Result.getBuilder();

  Builder->printUseDef(OS);
  Builder->printDefInfoMap(OS);

  debug::LLVMDG2Dot Dumper(Builder->getDG());
  Dumper.dump("dg.dot");

  Builder->dump(OS);

  return PreservedAnalyses::all();
}
