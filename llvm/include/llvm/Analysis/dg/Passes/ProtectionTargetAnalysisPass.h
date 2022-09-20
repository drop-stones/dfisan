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

using ValueSet = std::unordered_set<llvm::Value *>;
using InstSet = std::unordered_set<llvm::Instruction *>;

class ProtectionTargetAnalysisPass : public AnalysisInfoMixin<ProtectionTargetAnalysisPass> {
  friend AnalysisInfoMixin<ProtectionTargetAnalysisPass>;

  static AnalysisKey Key;

public:
  // Protection Target Info
  class Result {
  public:
    Result() {}

    void insertGlobalTarget(Value *G) { GlobalTargets.insert(G); }
    void insertLocalTarget (Value *L) { LocalTargets.insert(L); }
    void insertHeapTarget  (Value *H) { HeapTargets.insert(H); }

    ValueSet& getGlobalTargets() { return GlobalTargets; }
    ValueSet& getLocalTargets()  { return LocalTargets; }
    ValueSet& getHeapTargets()   { return HeapTargets; }

    void dump(raw_ostream &OS) {
      OS << "ProtectionTargetAnalysisPass Result::dump()\n";
      OS << " - GlobalTargets:\n";
      for (const auto *Global : GlobalTargets)
        OS << "  - " << *Global << "\n";
      for (const auto *Local : LocalTargets)
        OS << "  - " << *Local << "\n";
      for (const auto *Heap : HeapTargets)
        OS << "  - " << *Heap << "\n";
    }

  private:
    ValueSet GlobalTargets;
    ValueSet LocalTargets;
    ValueSet HeapTargets;
  };

  Result run(Module &M, ModuleAnalysisManager &MAM);

private:
  // Find "dfi_protection" annotations and collect targets
  void findProtectionTargetAnnotations(Module &M);

  Result Res;
  const std::string ProtectionAnno = "dfi_protection";
  const std::string SafeMallocFnName = "safe_malloc";
  const std::string SafeCallocFnName = "safe_calloc";
  const std::string SafeReallocFnName = "safe_realloc";
};

} // namespace llvm

#endif
