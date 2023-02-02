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
#include "llvm/Transforms/Utils/Cloning.h"

#include "ModulePass/ProtectionTargetAnalysisPass.h"

using namespace llvm;

#define DEBUG_TYPE "protection-target"

static llvm::cl::opt<bool> ProtectAll(
  "protect-all",
  llvm::cl::init(false),
  llvm::cl::desc("<protect all data>"),
  llvm::cl::Optional
);

namespace {
static inline bool isReplacementFnName(StringRef FnName) {
  return (FnName == SafeMallocFnName) || (FnName == SafeCallocFnName) || (FnName == SafeReallocFnName);
}

static inline bool isSafeAlignedAllocFnName(StringRef FnName) {
  return (FnName == SafeAlignedMallocFnName) || (FnName == SafeAlignedCallocFnName) || (FnName == SafeAlignedReallocFnName);
}

static inline bool isSafeUnalignedAllocFnName(StringRef FnName) {
  return (FnName == SafeUnalignedMallocFnName) || (FnName == SafeUnalignedCallocFnName) || (FnName == SafeUnalignedReallocFnName);
}

static inline bool isSafeAllocFnName(StringRef FnName) {
  return isSafeAlignedAllocFnName(FnName) || isSafeUnalignedAllocFnName(FnName);
}

// Collect global variables annotated by "dfi_protection".
void collectGlobalTargetsWithAnnotation(Module &M, ValueSet &GlobalTargets) {
  if (GlobalVariable *GlobAnno = M.getNamedGlobal("llvm.global.annotations")) {
    ConstantArray *CArr = dyn_cast<ConstantArray>(GlobAnno->getOperand(0));
    for (Value *CArrOp : CArr->operands()) {
      ConstantStruct *AnnoStruct = dyn_cast<ConstantStruct>(CArrOp);
      Value *GlobalTarget = AnnoStruct->getOperand(0)->getOperand(0);
      if (!isa<GlobalVariable>(GlobalTarget)) {
        LLVM_DEBUG(dbgs() << "Skip global target: " << *GlobalTarget << "\n");
        continue;
      }
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

void collectAllGlobals(Module &M, ValueSet &Globals) {
  // collect globals
  for (auto &Global : M.getGlobalList()) {
    if (GlobalVariable *GlobVar = dyn_cast<GlobalVariable>(&Global)) {
      if (GlobVar->isConstant())
        continue;
      LLVM_DEBUG(dbgs() << "Global: " << *GlobVar << "\n");
      // llvm::errs() << "Global: " << *GlobVar << "\n";
      Globals.insert(GlobVar);
    }
  }
}

// Collect all targets
void collectAllTargets(Module &M, ValueSet &Globals, ValueSet &Locals, ValueSet &Heaps) {
  static Function *SafeMalloc = nullptr;

  collectAllGlobals(M, Globals);

  for (Function &Func : M.getFunctionList()) {
    for (Instruction &Inst : instructions(&Func)) {
      if (AllocaInst *Alloca = dyn_cast<AllocaInst>(&Inst)) {
        LLVM_DEBUG(dbgs() << "Local: " << *Alloca << "\n");
        Locals.insert(Alloca);
      } else if (CallInst *Call = dyn_cast<CallInst>(&Inst)) {
        // Value *Callee = Call->getCalledOperand()->stripPointerCasts();
        Function *Fn = Call->getCalledFunction();
        if (Fn->getName() == "malloc") {
          if (SafeMalloc == nullptr) {
            ValueToValueMapTy Map;
            SafeMalloc = CloneFunction(Fn, Map, nullptr);
            SafeMalloc->setName("safe_malloc");
          }
          Call->setCalledFunction(SafeMalloc);
          LLVM_DEBUG(dbgs() << "Heap: " << *Call << "\n");
          llvm::errs() << "Heap: " << *Call << "\n";
          Heaps.insert(Call);
        }
      }
    }
  }
}

// Divide global targets into aligned and unaligned by their sections.
void collectGlobalTargetsBySection(Module &M, ValueSet &AlignedTargets, ValueSet &UnalignedTargets) {
  ValueSet GlobalTargets;
  if (ProtectAll)
    collectAllGlobals(M, GlobalTargets);
  else
    collectGlobalTargetsWithAnnotation(M, GlobalTargets);

  for (auto *Target : GlobalTargets) {
    assert(isa<GlobalVariable>(Target) && "Global target is not global variable");
    GlobalVariable *GlobVar = dyn_cast<GlobalVariable>(Target);
    if (GlobVar->getSection() == SafeAlignedGlobalSecName) {
      AlignedTargets.insert(Target);
    } else if (GlobVar->getSection() == SafeUnalignedGlobalSecName) {
      UnalignedTargets.insert(Target);
    }
  }
}

// Divide heap targets into aligned and unaligned by their alloc fn names.
void collectHeapTargetsByFnName(Module &M, ValueSet &AlignedTargets, ValueSet &UnalignedTargets) {
  for (Function &Func : M.getFunctionList()) {
    for (Instruction &Inst : instructions(&Func)) {
      if (CallInst *Call = dyn_cast<CallInst>(&Inst)) {
        Value *Callee = Call->getCalledOperand()->stripPointerCasts();
        if (isSafeAlignedAllocFnName(Callee->getName())) {
          LLVM_DEBUG(dbgs() << "Protection Target allocated by aligned safe alloc: " << *Call << "\n");
          AlignedTargets.insert(Call);
        } else if (isSafeUnalignedAllocFnName(Callee->getName())) {
          LLVM_DEBUG(dbgs() << "Protection Target allocated by unaligned safe alloc: " << *Call << "\n");
          UnalignedTargets.insert(Call);
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
  if (ProtectAll) {
    collectAllTargets(M, Res.getGlobalTargets(), Res.getLocalTargets(), Res.getHeapTargets());
  } else {
    // Collect annotated global variables
    collectGlobalTargetsWithAnnotation(M, Res.getGlobalTargets());
    // Collect annotated local variables
    collectLocalTargetsWithAnnotation(M, Res.getLocalTargets());
    // Collect safe heap variables
    collectHeapTargetsAllocatedByReplacementFn(M, Res.getHeapTargets());
  }
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
  collectGlobalTargetsBySection(M, Res.getAlignedTargets(), Res.getUnalignedTargets());

  // Collect protection targets allocated by safe allocs
  collectHeapTargetsByFnName(M, Res.getAlignedTargets(), Res.getUnalignedTargets());
}
