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

using namespace llvm;
using namespace dg;

// Provide an explicit template instantiation for the static ID.
AnalysisKey UseDefAnalysisPass::Key;

void printUseDef(raw_ostream &OS, LLVMDependenceGraph *DG) {
  auto *DDA = DG->getDDA();
  for (const auto &Iter : *DG) {
    auto *Val = Iter.first;
    auto *Node = Iter.second;
    if (DDA->isUse(Val)) {
      OS << "Use: " << *Val << "\n";
      for (const auto *Def : DDA->getLLVMDefinitions(Val)) {
        OS << " - Def: " << *Def << "\n";
      }
    } else if (DDA->isDef(Val)) {
      // llvm::errs() << "Def: " << *Val << "\n";
    }
  }
}

UseDefAnalysisPass::Result
UseDefAnalysisPass::run(Module &M, ModuleAnalysisManager &MAM) {
  std::unique_ptr<dg::UseDefBuilder> Builder = std::make_unique<dg::UseDefBuilder>(&M);
  Builder->buildDG();
  Builder->assignDefIDs();

  UseDefAnalysisPass::Result Result { std::move(Builder) };

  return Result;
}

PreservedAnalyses
UseDefPrinterPass::run(Module &M, ModuleAnalysisManager &MAM) {
  OS << "UseDefPrinterPass::print " << M.getName() << "\n";
  auto &Result = MAM.getResult<UseDefAnalysisPass>(M);
  auto &Builder = Result.getBuilder();

  Builder.printUseDef(OS);
  Builder.printDefInfoMap(OS);

  debug::LLVMDG2Dot Dumper(Builder.getDG());
  Dumper.dump("dg.dot");

  return PreservedAnalyses::all();
}
