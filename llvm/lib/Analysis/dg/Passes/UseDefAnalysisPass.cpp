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

using namespace llvm;
using namespace dg;

// Provide an explicit template instantiation for the static ID.
AnalysisKey UseDefAnalysisPass::Key;

UseDefAnalysisPass::Result
UseDefAnalysisPass::run(Module &M, ModuleAnalysisManager &MAM) {
  llvmdg::LLVMDependenceGraphBuilder Builder(&M);
  auto DG = Builder.build();

  return { DG.release() };
}

PreservedAnalyses
UseDefPrinterPass::run(Module &M, ModuleAnalysisManager &MAM) {
  OS << "UseDefPrinterPass::print " << M.getName() << "\n";
  auto Result = MAM.getResult<UseDefAnalysisPass>(M);
  debug::LLVMDG2Dot Dumper(Result.DG);
  Dumper.dump("dg.dot");
  // Dumper.dump((M.getName() + ".dot").str().c_str());

  return PreservedAnalyses::all();
}
