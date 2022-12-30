#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Instrumentation/ReplaceWithSafeAlloc.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Support/Debug.h"

// #include "dg/Passes/ProtectionTargetAnalysisPass.h"
#include "ModulePass/ProtectionTargetAnalysisPass.h"

#define DEBUG_TYPE "replace-with-safe-alloc"

using namespace llvm;

///
// Command-line flags
///

static cl::opt<bool> ClUnalignedRegionOnly(
  "unaligned-region-only", cl::desc("Protection targets are placed in unaligned region only"),
  cl::Hidden, cl::init(false));

namespace {

const DataLayout *DL;
Type *VoidTy, *Int64Ty, *Int8PtrTy;
FunctionType *MallocTy, *FreeTy, *CallocTy, *ReallocTy;
FunctionCallee SafeAlignedMalloc, SafeUnalignedMalloc, SafeAlignedFree, SafeUnalignedFree,
               SafeAlignedCalloc, SafeUnalignedCalloc, SafeAlignedRealloc, SafeUnalignedRealloc;

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

// Check whether the given Type should be in aligned region or not.
bool isInAlignedRegion(Type *TargetType) {
  if (ClUnalignedRegionOnly)  // if the option is on, all targets should be in unaligned region
    return false;
  return isFourAligned(TargetType);
}

void initTypes(Module &M) {
  DL = &(M.getDataLayout());

  VoidTy = Type::getVoidTy(M.getContext());
  Int64Ty = Type::getInt64Ty(M.getContext());
  Int8PtrTy = Type::getInt8PtrTy(M.getContext());

  MallocTy = FunctionType::get(Int8PtrTy, {Int64Ty}, false);
  SafeAlignedMalloc = M.getOrInsertFunction(SafeAlignedMallocFnName, MallocTy);
  SafeUnalignedMalloc = M.getOrInsertFunction(SafeUnalignedMallocFnName, MallocTy);

  FreeTy = FunctionType::get(VoidTy, {Int8PtrTy}, false);
  SafeAlignedFree = M.getOrInsertFunction(SafeAlignedFreeFnName, FreeTy);
  SafeUnalignedFree = M.getOrInsertFunction(SafeUnalignedFreeFnName, FreeTy);

  CallocTy = FunctionType::get(Int8PtrTy, {Int64Ty, Int64Ty}, false);
  SafeAlignedCalloc = M.getOrInsertFunction(SafeAlignedCallocFnName, CallocTy);
  SafeUnalignedCalloc = M.getOrInsertFunction(SafeUnalignedCallocFnName, CallocTy);

  ReallocTy = FunctionType::get(Int8PtrTy, {Int8PtrTy, Int64Ty}, false);
  SafeAlignedRealloc = M.getOrInsertFunction(SafeAlignedReallocFnName, ReallocTy);
  SafeUnalignedRealloc = M.getOrInsertFunction(SafeUnalignedReallocFnName, ReallocTy);
}

Value *createSafeAllocAndFree(Instruction *OrigInst, Type *OrigTy) {
  IRBuilder<> Builder(OrigInst);
  Value *NewVal = nullptr;

  if (AllocaInst *Alloca = dyn_cast<AllocaInst>(OrigInst)) {
    // Create safe malloc for local alloca
    TypeSize Size = DL->getTypeAllocSize(OrigTy);
    Constant *SizeVal = ConstantInt::get(Int64Ty, Size);
    CallInst *SafeAlloc = isInAlignedRegion(OrigTy) ? Builder.CreateCall(MallocTy, SafeAlignedMalloc.getCallee(), {SizeVal})
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
      isInAlignedRegion(OrigTy) ? Builder.CreateCall(SafeAlignedFree, SafeAlloc)
                                : Builder.CreateCall(SafeUnalignedFree, SafeAlloc);
    }
  } else if (CallInst *Call = dyn_cast<CallInst>(OrigInst)) {
    // Create safe alloc for heap alloc
    Value *Callee = Call->getCalledOperand()->stripPointerCasts();
    assert(Callee != nullptr && "Invalid Callee");
    if (Callee->getName() == "safe_malloc") {
      Value *SizeVal = Call->getArgOperand(0);
      NewVal = isInAlignedRegion(OrigTy) ? Builder.CreateCall(MallocTy, SafeAlignedMalloc.getCallee(), {SizeVal})
                                         : Builder.CreateCall(MallocTy, SafeUnalignedMalloc.getCallee(), {SizeVal});
    } else if (Callee->getName() == "safe_calloc") {
      Value *SizeVal = Call->getArgOperand(0);
      Value *ElemSizeVal = Call->getArgOperand(1);
      NewVal = isInAlignedRegion(OrigTy) ? Builder.CreateCall(CallocTy, SafeAlignedCalloc.getCallee(), {SizeVal, ElemSizeVal})
                                         : Builder.CreateCall(CallocTy, SafeUnalignedCalloc.getCallee(), {SizeVal, ElemSizeVal});
    } else if (Callee->getName() == "safe_realloc") {
      Value *PtrVal = Call->getArgOperand(0);
      Value *SizeVal = Call->getArgOperand(1);
      NewVal = isInAlignedRegion(OrigTy) ? Builder.CreateCall(ReallocTy, SafeAlignedRealloc.getCallee(), {PtrVal, SizeVal})
                                         : Builder.CreateCall(ReallocTy, SafeUnalignedRealloc.getCallee(), {PtrVal, SizeVal});
    } else {
      llvm_unreachable("No support function call");
    }
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
    assert(Callee != nullptr && "Invalid Callee");
    Value *NextUser = *Call->user_begin();
    if (NextUser == nullptr)  // Skip the unused target
      continue;
    Type *TargetType = nullptr;
    for (auto *User : Call->users()) {
      if (BitCastInst *Bitcast = dyn_cast<BitCastInst>(User)) {
        TargetType = Bitcast->getDestTy()->getPointerElementType();
      } else if (StoreInst *Store = dyn_cast<StoreInst>(User)) {
        TargetType = Store->getPointerOperandType()->getPointerElementType();
      } else {
        continue;
      }
      if (TargetType != nullptr)
        break;
    }
    LLVM_DEBUG(dbgs() << "Callee: " << Callee->getName() << ", Type: " << *TargetType << "\n");
    Value *SafeAlloc = createSafeAllocAndFree(Call, TargetType);
    replaceAndEraseInst(Call, SafeAlloc);
  }
}

void setSafeSectionToGlobalTargets(ValueSet &GlobalTargets) {
  for (auto *GlobalTarget : GlobalTargets) {
    assert(isa<GlobalVariable>(GlobalTarget) && "Invalid global value");
    GlobalVariable *GlobVar = dyn_cast<GlobalVariable>(GlobalTarget);
    // Set appropriate global section
    if (GlobVar->getLinkage() == GlobalValue::LinkageTypes::ExternalLinkage)
      if (GlobVar->getName() == "stdout")
        continue;
    Type *TargetType = GlobVar->getValueType();
    if (isInAlignedRegion(TargetType)) {
      GlobVar->setSection(SafeAlignedGlobalSecName);
      LLVM_DEBUG(dbgs() << "Set section " << SafeAlignedGlobalSecName << " for " << *GlobVar << "\n");
    } else {
      GlobVar->setSection(".safe_unaligned_global");
      LLVM_DEBUG(dbgs() << "Set section " << SafeUnalignedGlobalSecName << " for " << *GlobVar << "\n");
    }
    GlobVar->setAlignment(MaybeAlign{8}); // Enforce 8 byte alignment
  }
}
} // anonymous namespace

PreservedAnalyses
ReplaceWithSafeAllocPass::run(Module &M, ModuleAnalysisManager &MAM) {
  LLVM_DEBUG(dbgs() << "ReplaceWithSafeAllocPass: Replace protection target's allocs with safe allocs\n");
  auto &Result = MAM.getResult<ProtectionTargetAnalysisPass>(M);

  initTypes(M);
  replaceLocalAllocsWithSafeAllocs(Result.getLocalTargets());
  replaceHeapAllocsWithSafeAllocs(Result.getHeapTargets());
  setSafeSectionToGlobalTargets(Result.getGlobalTargets());

  return PreservedAnalyses::none();
}
