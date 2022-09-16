//===-- ProtectionTargetAnalysisPass.h - Annotation Analysis Pass-*- C++ -*-===//
//
//===----------------------------------------------------------------------===//
///
/// This file contains the declaration of the ProtectionTargetAnalysis class,
/// which collect "dfi_protection" annotations.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_DG_PASSES_PROTECTIONTARGETANALYSIS_PASS_H
#define LLVM_ANALYSIS_DG_PASSES_PROTECTIONTARGETANALYSIS_PASS_H

#include "llvm/IR/PassManager.h"
#include <unordered_set>

namespace llvm {

using ValueSet = std::unordered_set<const llvm::Value *>;

class ProtectionTargetAnalysisPass : public AnalysisInfoMixin<ProtectionTargetAnalysisPass> {
  friend AnalysisInfoMixin<ProtectionTargetAnalysisPass>;

  static AnalysisKey Key;

public:
  // Protection Target Info
  class Result {
  public:
    Result() {}

    void insertGlobalTarget(const Value *G) { GlobalTargets.insert(G); }
    void insertLocalTarget (const Value *L) { LocalTargets.insert(L); }

    ValueSet& getGlobalTargets() { return GlobalTargets; }
    ValueSet& getLocalTargets()  { return LocalTargets; }

    void dump(raw_ostream &OS) {
      OS << "ProtectionTargetAnalysisPass Result::dump()\n";
      OS << " - GlobalTargets:\n";
      for (const auto *Global : GlobalTargets)
        OS << "  - " << *Global << "\n";
      for (const auto *Local : LocalTargets)
        OS << "  - " << *Local << "\n";
    }

  private:
    ValueSet GlobalTargets;
    ValueSet LocalTargets;
  };

  Result run(Module &M, ModuleAnalysisManager &MAM);

private:
  // Find "dfi_protection" annotations and collect targets
  void findProtectionTargetAnnotations(Module &M);

  Result Res;
  const std::string ProtectionAnno = "dfi_protection";
};

} // namespace llvm

#endif
