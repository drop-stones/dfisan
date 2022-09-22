//===-- ProtectionTargetAnalysisPass.h - Annotation Analysis Pass-*- C++ -*-===//
//
//===----------------------------------------------------------------------===//
///
/// This file contains the declaration of the ProtectionTargetAnalysis class,
/// which collect "dfi_protection" annotations and analyze protection targets.
///
//===----------------------------------------------------------------------===//

#include "llvm/IR/Constants.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/Debug.h"

#include "dg/Passes/ProtectionTargetAnalysisPass.h"

using namespace llvm;

#define DEBUG_TYPE "protection-target"

namespace {
static inline bool isReplacementFnName(StringRef FnName) {
  return (FnName == SafeMallocFnName) || (FnName == SafeCallocFnName) || (FnName == SafeReallocFnName);
}

static inline bool isSafeAllocFnName(StringRef FnName) {
  return (FnName == SafeAlignedMallocFnName) || (FnName == SafeUnalignedMallocFnName) ||
         (FnName == SafeAlignedCallocFnName) || (FnName == SafeUnalignedCallocFnName) ||
         (FnName == SafeAlignedReallocFnName) || (FnName == SafeUnalignedReallocFnName);
}

// Collect global variables annotated by "dfi_protection".
void collectGlobalTargetsWithAnnotation(Module &M, ValueSet &GlobalTargets) {
  if (GlobalVariable *GlobAnno = M.getNamedGlobal("llvm.global.annotations")) {
    ConstantArray *CArr = dyn_cast<ConstantArray>(GlobAnno->getOperand(0));
    for (Value *CArrOp : CArr->operands()) {
      ConstantStruct *AnnoStruct = dyn_cast<ConstantStruct>(CArrOp);
      Value *GlobalTarget = AnnoStruct->getOperand(0)->getOperand(0);
      GlobalVariable *Anno = dyn_cast<GlobalVariable>(AnnoStruct->getOperand(1)->getOperand(0));
      ConstantDataArray *AnnoCharArr = dyn_cast<ConstantDataArray>(Anno->getOperand(0));
      StringRef AnnoName = AnnoCharArr->getAsString();
      if (AnnoName.startswith(ProtectionAnnoName)) {
        LLVM_DEBUG(dbgs() << "Global Target: " << *GlobalTarget << "\n");
        GlobalTargets.insert(GlobalTarget);
      }
    }
  }
}

// Collect local variables annotated by "dfi_protection".
void collectLocalTargetsWithAnnotation(Module &M, ValueSet &LocalTargets) {
  for (Function &Func : M.getFunctionList()) {
    for (Instruction &Inst : instructions(&Func)) {
      if (IntrinsicInst *Intrinsic = dyn_cast<IntrinsicInst>(&Inst)) {
        Function *Callee = Intrinsic->getCalledFunction();
        if (Callee->getName() == "llvm.var.annotation") {
          Value *LocalTarget = Intrinsic->getArgOperand(0)->stripPointerCasts();
          if (!isa<AllocaInst>(LocalTarget))
            continue;
          GlobalVariable *Anno = dyn_cast<GlobalVariable>(Intrinsic->getOperand(1)->stripPointerCasts());
          ConstantDataArray *AnnoCharArr = dyn_cast<ConstantDataArray>(Anno->getOperand(0));
          StringRef AnnoName = AnnoCharArr->getAsString();
          if (AnnoName.startswith(ProtectionAnnoName)) {
            LLVM_DEBUG(dbgs() << "Local Target: " << *LocalTarget << "\n");
            LocalTargets.insert(LocalTarget);
          }
        }
      }
    }
  }
}

// Collect heap targets allocated by "safe_malloc", "safe_calloc" and "safe_realloc".
void collectHeapTargetsAllocatedByReplacementFn(Module &M, ValueSet &HeapTargets) {
  for (Function &Func : M.getFunctionList()) {
    for (Instruction &Inst : instructions(&Func)) {
      if (CallInst *Call = dyn_cast<CallInst>(&Inst)) {
        Value *Callee = Call->getCalledOperand()->stripPointerCasts();
        if (isReplacementFnName(Callee->getName())) {
          LLVM_DEBUG(dbgs() << "Heap Target: " << *Call << "\n");
          HeapTargets.insert(Call);
        }
      }
    }
  }
}

// Collect heap targets allocated by safe allocs.
void collectHeapTargetsAllocatedBySafeAllocFn(Module &M, ValueSet &HeapTargets) {
  for (Function &Func : M.getFunctionList()) {
    for (Instruction &Inst : instructions(&Func)) {
      if (CallInst *Call = dyn_cast<CallInst>(&Inst)) {
        Value *Callee = Call->getCalledOperand()->stripPointerCasts();
        if (isSafeAllocFnName(Callee->getName())) {
          LLVM_DEBUG(dbgs() << "Protection Target allocated by safe alloc: " << *Call << "\n");
          // Res.insertProtectionTarget(Call);
          HeapTargets.insert(Call);
        }
      }
    }
  }
}
} // anonymous namespace

/* --- ProtectionTargetAnalysisPass --- */
// Provide an explicit template instantiation for the static ID.
AnalysisKey ProtectionTargetAnalysisPass::Key;

ProtectionTargetAnalysisPass::Result
ProtectionTargetAnalysisPass::run(Module &M, ModuleAnalysisManager &MAM) {
  LLVM_DEBUG(dbgs() << "ProtectionTargetAnalysisPass::" << __func__ << "\n");
  ProtectionTargetAnalysisPass::Result Res;

  findProtectionTargetAnnotations(M, Res);

  return Res;
}

void
ProtectionTargetAnalysisPass::findProtectionTargetAnnotations(Module &M, Result &Res) {
  // Collect annotated global variables
  collectGlobalTargetsWithAnnotation(M, Res.getGlobalTargets());

  // Collect annotated local variables
  collectLocalTargetsWithAnnotation(M, Res.getLocalTargets());

  // Collect safe heap variables
  collectHeapTargetsAllocatedByReplacementFn(M, Res.getHeapTargets());
}

/* --- CollectProtectionTargetPass --- */
// Provide an explicit templace instantiation for the static ID.
AnalysisKey CollectProtectionTargetPass::Key;

CollectProtectionTargetPass::Result
CollectProtectionTargetPass::run(Module &M, ModuleAnalysisManager &MAM) {
  LLVM_DEBUG(dbgs() << "CollectProtectionTargetPass::" << __func__ << "\n");
  CollectProtectionTargetPass::Result Res;
  findProtectionTargets(M, Res);
  return Res;
}

void
CollectProtectionTargetPass::findProtectionTargets(Module &M, Result &Res) {
  // Collect global targets
  collectGlobalTargetsWithAnnotation(M, Res.getProtectionTargets());

  // Collect protection targets allocated by safe allocs
  collectHeapTargetsAllocatedBySafeAllocFn(M, Res.getProtectionTargets());
}
