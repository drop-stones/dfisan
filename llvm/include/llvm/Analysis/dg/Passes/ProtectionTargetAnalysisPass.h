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

constexpr char SafeMallocFnName[]           = "safe_malloc";
constexpr char SafeCallocFnName[]           = "safe_calloc";
constexpr char SafeReallocFnName[]          = "safe_realloc";
constexpr char SafeAlignedMallocFnName[]    = "__dfisan_safe_aligned_malloc";
constexpr char SafeUnalignedMallocFnName[]  = "__dfisan_safe_unaligned_malloc";
constexpr char SafeAlignedFreeFnName[]      = "__dfisan_safe_aligned_free";
constexpr char SafeUnalignedFreeFnName[]    = "__dfisan_safe_unaligned_free";
constexpr char SafeAlignedCallocFnName[]    = "__dfisan_safe_aligned_calloc";
constexpr char SafeUnalignedCallocFnName[]  = "__dfisan_safe_unaligned_calloc";
constexpr char SafeAlignedReallocFnName[]   = "__dfisan_safe_aligned_realloc";
constexpr char SafeUnalignedReallocFnName[] = "__dfisan_safe_unaligned_realloc";

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
    Result() : BeforeReplacement(false) {}

    void insertGlobalTarget(Value *G) { GlobalTargets.insert(G); }
    void insertLocalTarget (Value *L) { LocalTargets.insert(L); }
    void insertHeapTarget  (Value *H) { HeapTargets.insert(H); }
    void insertProtectionTarget(Value *P) { ProtectionTargets.insert(P); }

    ValueSet& getGlobalTargets() { return GlobalTargets; }
    ValueSet& getLocalTargets()  { return LocalTargets; }
    ValueSet& getHeapTargets()   { return HeapTargets; }
    ValueSet& getProtectionTargets() { return ProtectionTargets; }

    void setBeforeReplacement(bool B) { BeforeReplacement = B; }
    bool beforeReplacement() { return BeforeReplacement; }

    void dump(raw_ostream &OS) {
      OS << "ProtectionTargetAnalysisPass Result::dump()\n";
      if (beforeReplacement()) {
        OS << " - GlobalTargets:\n";
        for (const auto *Global : GlobalTargets)
          OS << "  - " << *Global << "\n";
        OS << " - LocalTargets:\n";
        for (const auto *Local : LocalTargets)
          OS << "  - " << *Local << "\n";
        OS << " - HeapTargets:\n";
        for (const auto *Heap : HeapTargets)
          OS << "  - " << *Heap << "\n";
      } else {
        OS << " - ProtectionTargets:\n";
        for (const auto *Target : ProtectionTargets)
          OS << "  - " << *Target << "\n";
      }
    }

  private:
    ValueSet GlobalTargets;     // Annotated global targets
    ValueSet LocalTargets;      // Annotated local targets
    ValueSet HeapTargets;       // Annotated heap targets
    ValueSet ProtectionTargets; // Protection targets allocated by safe allocs
    bool BeforeReplacement;
  };

  Result run(Module &M, ModuleAnalysisManager &MAM);

private:
  // Find "dfi_protection" annotations and collect targets
  void findProtectionTargetAnnotations(Module &M, Result &Res);

  // Find protection targets allocated by safe allocs
  void findProtectionTargets(Module &M, Result &Res);

  // Result Res;
  const std::string ProtectionAnno = "dfi_protection";
};

} // namespace llvm

#endif
