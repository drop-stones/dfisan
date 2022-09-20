#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Instrumentation/ReplaceWithSafeAlloc.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Support/Debug.h"

#include "dg/Passes/ProtectionTargetAnalysisPass.h"

#define DEBUG_TYPE "replace-with-safe-alloc"

using namespace llvm;

namespace {

const DataLayout *DL;
Type *VoidTy, *Int64Ty, *Int8PtrTy;
FunctionType *MallocTy, *FreeTy;
FunctionCallee SafeAlignedMalloc, SafeUnalignedMalloc, SafeAlignedFree, SafeUnalignedFree;

// Check whether the given Type is four aligned or not.
bool isFourAligned(Type *TargetType) {
  LLVM_DEBUG(dbgs() << __func__ << "( " << *TargetType << " )\n");
  bool Ret = true;
  if (auto *StructTy = dyn_cast<StructType>(TargetType)) {
    auto *StructLayout = DL->getStructLayout(StructTy);
    LLVM_DEBUG(dbgs() << "Align: " << StructLayout->getAlignment().value() << "\n");
    for (const auto EleOff : StructLayout->getMemberOffsets()) {
      LLVM_DEBUG(dbgs() << " - EleOff: " << EleOff << "\n");
      if (EleOff % 4 != 0)
        Ret = false;
    }
  }
  LLVM_DEBUG(dbgs() << (Ret ? "Four Align" : "Not Four Align") << "\n");
  return Ret;
}

void initTypes(Module &M) {
  DL = &(M.getDataLayout());
  VoidTy = Type::getVoidTy(M.getContext());
  Int64Ty = Type::getInt64Ty(M.getContext());
  Int8PtrTy = Type::getInt8PtrTy(M.getContext());
  MallocTy = FunctionType::get(Int8PtrTy, {Int64Ty}, false);
  SafeAlignedMalloc = M.getOrInsertFunction("__dfisan_safe_aligned_malloc", MallocTy);
  SafeUnalignedMalloc = M.getOrInsertFunction("__dfisan_safe_unaligned_malloc", MallocTy);
  FreeTy = FunctionType::get(VoidTy, {Int8PtrTy}, false);
  SafeAlignedFree = M.getOrInsertFunction("__dfisan_safe_aligned_free", FreeTy);
  SafeUnalignedFree = M.getOrInsertFunction("__dfisan_safe_unaligned_free", FreeTy);
}

Value *createSafeAllocAndFree(Instruction *OrigInst, Type *OrigTy) {
  IRBuilder<> Builder(OrigInst);
  Value *NewVal = nullptr;

  if (AllocaInst *Alloca = dyn_cast<AllocaInst>(OrigInst)) {
    // Create safe malloc for local alloca
    TypeSize Size = DL->getTypeAllocSize(OrigTy);
    Constant *SizeVal = ConstantInt::get(Int64Ty, Size);
    CallInst *SafeAlloc = isFourAligned(OrigTy) ? Builder.CreateCall(MallocTy, SafeAlignedMalloc.getCallee(), {SizeVal})
                                                : Builder.CreateCall(MallocTy, SafeUnalignedMalloc.getCallee(), {SizeVal});
    NewVal = Builder.CreateBitCast(SafeAlloc, Alloca->getType(), Alloca->getName() + "_replaced");

    // Insert free for safe malloc before return
    InstSet Rets;
    // Find every return instruction
    for (Instruction &Inst : instructions(OrigInst->getFunction())) {
      if (auto *Ret = dyn_cast<ReturnInst>(&Inst)) {
        Rets.insert(Ret);
      }
    }
    // Insert free before return
    for (auto *Ret : Rets) {
      Builder.SetInsertPoint(Ret);
      isFourAligned(OrigTy) ? Builder.CreateCall(SafeAlignedFree, SafeAlloc)
                            : Builder.CreateCall(SafeUnalignedFree, SafeAlloc);
    }
  } else if (CallInst *Call = dyn_cast<CallInst>(OrigInst)) {
    // Create safe malloc for heap alloc
    Value *Callee = Call->getCalledOperand()->stripPointerCasts();
    assert((Callee->getName() == "safe_malloc") && "Invalid call");
    Value *SizeVal = Call->getArgOperand(0);
    NewVal = isFourAligned(OrigTy) ? Builder.CreateCall(MallocTy, SafeAlignedMalloc.getCallee(), {SizeVal})
                                   : Builder.CreateCall(MallocTy, SafeUnalignedMalloc.getCallee(), {SizeVal});
  } else {
    llvm_unreachable("No Support code pattern");
  }
  return NewVal;
}

void replaceAndEraseInst(Instruction *From, Value *To) {
  LLVM_DEBUG(dbgs() << "replace " << *From << " with " << *To << "\n");
  From->replaceAllUsesWith(To);
  From->eraseFromParent();
}

void replaceLocalAllocsWithSafeAllocs(ValueSet &LocalTargets) {
  for (auto *LocalTarget : LocalTargets) {
    assert(isa<AllocaInst>(LocalTarget) && "Invalid local value");
    AllocaInst *Alloca = dyn_cast<AllocaInst>(LocalTarget);
    Type *TargetType = Alloca->getAllocatedType();
    LLVM_DEBUG(dbgs() << "Alloca: " << *Alloca << ", Type: " << *TargetType << "\n");
    Value *SafeAlloc = createSafeAllocAndFree(Alloca, TargetType);
    replaceAndEraseInst(Alloca, SafeAlloc);
  }
}

void replaceHeapAllocsWithSafeAllocs(ValueSet &HeapTargets) {
  for (auto *HeapTarget : HeapTargets) {
    assert(isa<CallInst>(HeapTarget) && "Invalid heap value");
    CallInst *Call = dyn_cast<CallInst>(HeapTarget);
    Value *Callee = Call->getCalledOperand()->stripPointerCasts();
    assert((Callee->getName() == "safe_malloc") && "Invalid call");
    Value *NextUser = *Call->user_begin();
    assert((isa<BitCastInst>(NextUser) && NextUser->getType()->isPointerTy()) && "No Support code pattern");
    Type *TargetType = dyn_cast<BitCastInst>(NextUser)->getDestTy()->getPointerElementType();
    LLVM_DEBUG(dbgs() << "Callee: " << Callee->getName() << ", Type: " << *TargetType << "\n");
    Value *SafeAlloc = createSafeAllocAndFree(Call, TargetType);
    replaceAndEraseInst(Call, SafeAlloc);
  }
}

void replaceGlobalAllocsWithSafeAllocs(Module &M, ValueSet &GlobalTargets) {
  // TODO: replace all global variables
  LLVM_DEBUG(dbgs() << "TODO: Replace all globals\n");
}
} // anonymous namespace

PreservedAnalyses
ReplaceWithSafeAllocPass::run(Module &M, ModuleAnalysisManager &MAM) {
  LLVM_DEBUG(dbgs() << "ReplaceWithSafeAllocPass: Replace protection target's allocs with safe allocs\n");
  auto &Result = MAM.getResult<ProtectionTargetAnalysisPass>(M);

  initTypes(M);
  replaceLocalAllocsWithSafeAllocs(Result.getLocalTargets());
  replaceHeapAllocsWithSafeAllocs(Result.getHeapTargets());

  return PreservedAnalyses::all();
}
