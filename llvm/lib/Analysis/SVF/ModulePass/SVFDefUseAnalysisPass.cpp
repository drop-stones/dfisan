//===-- SVFDefUseAnalysisPass.h - Def-Use Analysis Pass definition -*- C++ -*-===//
//
//===-------------------------------------------------------------------------===//
///
/// This file contains the declaration of the SVFDefUseAnalysisPass class,
/// which run Use-Def analysis and save the results to ModuleAnalysisManager.
///
//===-------------------------------------------------------------------------===//

#include "SVF-FE/LLVMModule.h"
#include "MTA/MHP.h"
#include "MTA/FSMPTA.h"
#include "ModulePass/SVFDefUseAnalysisPass.h"
#include "ModulePass/ProtectionTargetAnalysisPass.h"
#include "Dfisan/ProtectInfo.h"
#include "Dfisan/DfisanMTA.h"
#include "Dfisan/DfisanSVFGBuilder.h"
#include "Dfisan/DefUseSolver.h"
#include "Util/Options.h"

using namespace llvm;
using namespace SVF;

// Provide an explicit template instantiation for the static ID.
AnalysisKey SVFDefUseAnalysisPass::Key;

bool SVFDefUseAnalysisPass::Result::emptyResult() {
  return ProtInfo->emptyTarget();
}

SVFDefUseAnalysisPass::Result
SVFDefUseAnalysisPass::run(Module &M, ModuleAnalysisManager &MAM) {
  SVFModule *SvfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(M);
  SvfModule->buildSymbolTableInfo();

  // Run MTA analysis
  MTA *Mta = new DfisanMTA();
  MHP *Mhp = Mta->computeMHP(SvfModule);
  LockAnalysis *LockAna = Mta->computeLocksets(Mhp->getTCT());

  // Run Flow-sensitive PTA for multithread programs
  FSMPTA *Fsmpta = new FSMPTA(Mhp, LockAna);
  if (!Options::PStat)
    Fsmpta->disablePrintStat();
  Fsmpta->initialize(SvfModule);
  Fsmpta->analyze();

  // Build Full SVFG
  DfisanSVFGBuilder Builder(Mhp, LockAna);
  SVFG *Svfg = Builder.buildFullSVFG(Fsmpta);

  // Prepare analysis result
  auto &AnalysisResult = MAM.getResult<CollectProtectionTargetPass>(M);
  ProtectInfo *ProtInfo = new ProtectInfo(AnalysisResult.getAlignedTargets(), AnalysisResult.getUnalignedTargets());
  
  // DefUse analysis
  DefUseSolver Solver(Svfg, Mhp, LockAna, ProtInfo);
  Solver.solve();
  Solver.collectUnsafeInst();

  Result Res{ProtInfo};

  return Res;
}

PreservedAnalyses
SVFDefUsePrinterPass::run(Module &M, ModuleAnalysisManager &MAM) {
  OS << "SVFDefUsePrinterPass::print " << M.getName() << "\n";
  auto &Result = MAM.getResult<SVFDefUseAnalysisPass>(M);
  Result.getProtectInfo()->dump(OS);

  return PreservedAnalyses::all();
}
