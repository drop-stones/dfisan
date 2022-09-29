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

  auto &AnalysisResult = MAM.getResult<CollectProtectionTargetPass>(M);
  std::unique_ptr<dg::UseDefBuilder> Builder = std::make_unique<dg::UseDefBuilder>(&M, AnalysisResult.getAlignedTargets(), AnalysisResult.getUnalignedTargets());
  if (Builder->getProtectInfo()->emptyTarget()) { // skip use-def analysis
    UseDefAnalysisPass::Result Result{ std::move(Builder->moveProtectInfo()) };
    return Result;
  }

  Builder->buildDG();

  Builder->getProtectInfo()->dump(llvm::outs());

  debug::UseDefLogger Logger{M};
  Logger.logDefInfo(Builder->getDDA(), Builder->getProtectInfo());

  UseDefAnalysisPass::Result Result { std::move(Builder->moveDG()), std::move(Builder->moveDgBuilder()), std::move(Builder->moveProtectInfo()) };

  return Result;
}

namespace {
static void printUseDefFromDDA(raw_ostream &OS, InstSet &Uses, dg::dda::LLVMDataDependenceAnalysis *DDA, dg::DfiProtectInfo *ProtectInfo) {
  for (auto *Use : Uses) {
    OS << "Use: " << *Use << "\n";
    for (auto *Def : DDA->getLLVMDefinitions(Use))
      OS << " - DefID[" << ProtectInfo->getDefID(Def) << "] " << *Def << "\n";
  }
}
} // anonymous namespace

PreservedAnalyses
UseDefPrinterPass::run(Module &M, ModuleAnalysisManager &MAM) {
  OS << "UseDefPrinterPass::print " << M.getName() << "\n";
  auto &Result = MAM.getResult<UseDefAnalysisPass>(M);
  if (Result.emptyResult()) // skip printer pass
    return PreservedAnalyses::all();

  auto *DG = Result.getDG();
  auto *DDA = DG->getDDA();
  auto *ProtectInfo = Result.getProtectInfo();

  debug::LLVMDG2Dot Dumper(DG);
  Dumper.dump("dg.dot");

  // Print use-def
  printUseDefFromDDA(OS, ProtectInfo->AlignedOnlyUses, DDA, ProtectInfo);
  printUseDefFromDDA(OS, ProtectInfo->UnalignedOnlyUses, DDA, ProtectInfo);
  printUseDefFromDDA(OS, ProtectInfo->BothOnlyUses, DDA, ProtectInfo);
  printUseDefFromDDA(OS, ProtectInfo->AlignedOrNoTargetUses, DDA, ProtectInfo);
  printUseDefFromDDA(OS, ProtectInfo->UnalignedOrNoTargetUses, DDA, ProtectInfo);
  printUseDefFromDDA(OS, ProtectInfo->BothOrNoTargetUses, DDA, ProtectInfo);

  return PreservedAnalyses::all();
}
