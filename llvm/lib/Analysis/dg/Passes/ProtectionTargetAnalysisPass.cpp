//===-- ProtectionTargetAnalysisPass.h - Annotation Analysis Pass-*- C++ -*-===//
//
//===----------------------------------------------------------------------===//
///
/// This file contains the declaration of the ProtectionTargetAnalysis class,
/// which collect "dfi_protection" annotations.
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
} // anonymous namespace

// Provide an explicit template instantiation for the static ID.
AnalysisKey ProtectionTargetAnalysisPass::Key;

ProtectionTargetAnalysisPass::Result
ProtectionTargetAnalysisPass::run(Module &M, ModuleAnalysisManager &MAM) {
  LLVM_DEBUG(dbgs() << "ProtectionTargetAnalysisPass::" << __func__ << "\n");
  ProtectionTargetAnalysisPass::Result Res;

  findProtectionTargetAnnotations(M, Res);
  if (!Res.beforeReplacement())
    findProtectionTargets(M, Res);

  return Res;
}

void
ProtectionTargetAnalysisPass::findProtectionTargetAnnotations(Module &M, Result &Res) {
  // Find annotated global variables
  if (GlobalVariable *GlobAnno = M.getNamedGlobal("llvm.global.annotations")) {
    ConstantArray *CArr = dyn_cast<ConstantArray>(GlobAnno->getOperand(0));
    for (Value *CArrOp : CArr->operands()) {
      ConstantStruct *AnnoStruct = dyn_cast<ConstantStruct>(CArrOp);
      Value *GlobalTarget = AnnoStruct->getOperand(0)->getOperand(0);
      GlobalVariable *Anno = dyn_cast<GlobalVariable>(AnnoStruct->getOperand(1)->getOperand(0));
      ConstantDataArray *AnnoCharArr = dyn_cast<ConstantDataArray>(Anno->getOperand(0));
      StringRef AnnoName = AnnoCharArr->getAsString();
      if (AnnoName.startswith(ProtectionAnno)) {
        LLVM_DEBUG(dbgs() << "Global Target: " << *GlobalTarget << "\n");
        Res.insertGlobalTarget(GlobalTarget);
      }
    }
  }

  // Find annotated local variables
  InstSet ToBeRemoved;
  for (Function &Func : M.getFunctionList()) {
    for (Instruction &Inst : instructions(&Func)) {
      if (IntrinsicInst *Intrinsic = dyn_cast<IntrinsicInst>(&Inst)) {
        Function *Callee = Intrinsic->getCalledFunction();
        if (Callee->getName() == "llvm.var.annotation") {
          Value *LocalTarget = Intrinsic->getArgOperand(0)->stripPointerCasts();
          GlobalVariable *Anno = dyn_cast<GlobalVariable>(Intrinsic->getOperand(1)->stripPointerCasts());
          ConstantDataArray *AnnoCharArr = dyn_cast<ConstantDataArray>(Anno->getOperand(0));
          StringRef AnnoName = AnnoCharArr->getAsString();
          if (AnnoName.startswith(ProtectionAnno)) {
            LLVM_DEBUG(dbgs() << "Local Target: " << *LocalTarget << "\n");
            Res.insertLocalTarget(LocalTarget);
            ToBeRemoved.insert(Intrinsic);
          }
        }
      }
    }
  }
  // Remove local annotations because unnecessary
  for (auto *Removed : ToBeRemoved)
    Removed->eraseFromParent();

  // Find safe heap variables
  for (Function &Func : M.getFunctionList()) {
    for (Instruction &Inst : instructions(&Func)) {
      if (CallInst *Call = dyn_cast<CallInst>(&Inst)) {
        Value *Callee = Call->getCalledOperand()->stripPointerCasts();
        if (isReplacementFnName(Callee->getName())) {
          LLVM_DEBUG(dbgs() << "Heap Target: " << *Call << "\n");
          Res.insertHeapTarget(Call);
        }
      }
    }
  }

  // TODO: Replacement of global targets
  // Res.setBeforeReplacement(!Res.getGlobalTargets().empty() || !Res.getHeapTargets().empty() || !Res.getLocalTargets().empty());
  Res.setBeforeReplacement(!Res.getHeapTargets().empty() || !Res.getLocalTargets().empty());
}

void
ProtectionTargetAnalysisPass::findProtectionTargets(Module &M, Result &Res) {
  // Find protection targets allocated by safe allocs
  for (Function &Func : M.getFunctionList()) {
    for (Instruction &Inst : instructions(&Func)) {
      if (CallInst *Call = dyn_cast<CallInst>(&Inst)) {
        Value *Callee = Call->getCalledOperand()->stripPointerCasts();
        if (isSafeAllocFnName(Callee->getName())) {
          LLVM_DEBUG(dbgs() << "Protection Target: " << *Call << "\n");
          Res.insertProtectionTarget(Call);
        }
      }
    }
  }
}
