#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/TypeFinder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Transforms/Instrumentation/DataFlowIntegritySanitizer.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"  /* appendToUsed */
#include "llvm/Transforms/Utils/BasicBlockUtils.h"  /* SplitBlockAndInsertIfThen */
#include "llvm/Support/Debug.h"
#include "llvm/ADT/Statistic.h"

#include "dg/Passes/UseDefAnalysisPass.h"
#include "dg/Passes/UseDefBuilder.h"
#include "dg/Passes/DfiUtils.h"
#include "dg/AnalysisOptions.h"

#define DEBUG_TYPE "dfi-instrument"

using namespace llvm;

constexpr char DfisanModuleCtorName[] = "dfisan.module_ctor";
constexpr char DfisanInitFnName[]     = "__dfisan_init";

/// Set function names
constexpr char CommonDfisanStoreFnName[] = "__dfisan_store_id";
constexpr char DfisanStoreNFnName[]      = "__dfisan_store_id_n";
constexpr char DfisanStore1FnName[]      = "__dfisan_store_id_1";
constexpr char DfisanStore2FnName[]      = "__dfisan_store_id_2";
constexpr char DfisanStore4FnName[]      = "__dfisan_store_id_4";
constexpr char DfisanStore8FnName[]      = "__dfisan_store_id_8";
constexpr char DfisanStore16FnName[]     = "__dfisan_store_id_16";
// aligned
constexpr char DfisanAlignedStoreNFnName[]                  = "__dfisan_aligned_store_id_n";
constexpr char DfisanAlignedStore1FnName[]                  = "__dfisan_aligned_store_id_1";
constexpr char DfisanAlignedStore2FnName[]                  = "__dfisan_aligned_store_id_2";
constexpr char DfisanAlignedStore4FnName[]                  = "__dfisan_aligned_store_id_4";
constexpr char DfisanAlignedStore8FnName[]                  = "__dfisan_aligned_store_id_8";
constexpr char DfisanAlignedStore16FnName[]                 = "__dfisan_aligned_store_id_16";
// unaligned
constexpr char DfisanUnalignedStoreNFnName[]                = "__dfisan_unaligned_store_id_n";
constexpr char DfisanUnalignedStore1FnName[]                = "__dfisan_unaligned_store_id_1";
constexpr char DfisanUnalignedStore2FnName[]                = "__dfisan_unaligned_store_id_2";
constexpr char DfisanUnalignedStore4FnName[]                = "__dfisan_unaligned_store_id_4";
constexpr char DfisanUnalignedStore8FnName[]                = "__dfisan_unaligned_store_id_8";
constexpr char DfisanUnalignedStore16FnName[]               = "__dfisan_unaligned_store_id_16";
// aligned or unaligned
constexpr char DfisanAlignedOrUnalignedStoreNFnName[]       = "__dfisan_aligned_or_unaligned_store_id_n";
constexpr char DfisanAlignedOrUnalignedStore1FnName[]       = "__dfisan_aligned_or_unaligned_store_id_1";
constexpr char DfisanAlignedOrUnalignedStore2FnName[]       = "__dfisan_aligned_or_unaligned_store_id_2";
constexpr char DfisanAlignedOrUnalignedStore4FnName[]       = "__dfisan_aligned_or_unaligned_store_id_4";
constexpr char DfisanAlignedOrUnalignedStore8FnName[]       = "__dfisan_aligned_or_unaligned_store_id_8";
constexpr char DfisanAlignedOrUnalignedStore16FnName[]      = "__dfisan_aligned_or_unaligned_store_id_16";
// conditional aligned (for aligned or no-target def)
constexpr char DfisanCondAlignedStoreNFnName[]              = "__dfisan_cond_aligned_store_id_n";
constexpr char DfisanCondAlignedStore1FnName[]              = "__dfisan_cond_aligned_store_id_1";
constexpr char DfisanCondAlignedStore2FnName[]              = "__dfisan_cond_aligned_store_id_2";
constexpr char DfisanCondAlignedStore4FnName[]              = "__dfisan_cond_aligned_store_id_4";
constexpr char DfisanCondAlignedStore8FnName[]              = "__dfisan_cond_aligned_store_id_8";
constexpr char DfisanCondAlignedStore16FnName[]             = "__dfisan_cond_aligned_store_id_16";
// conditional unaligned (for unaligned or no-target def)
constexpr char DfisanCondUnalignedStoreNFnName[]            = "__dfisan_cond_unaligned_store_id_n";
constexpr char DfisanCondUnalignedStore1FnName[]            = "__dfisan_cond_unaligned_store_id_1";
constexpr char DfisanCondUnalignedStore2FnName[]            = "__dfisan_cond_unaligned_store_id_2";
constexpr char DfisanCondUnalignedStore4FnName[]            = "__dfisan_cond_unaligned_store_id_4";
constexpr char DfisanCondUnalignedStore8FnName[]            = "__dfisan_cond_unaligned_store_id_8";
constexpr char DfisanCondUnalignedStore16FnName[]           = "__dfisan_cond_unaligned_store_id_16";
// conditional aligned or unaligned (for aligned or unaligend or no-target def)
constexpr char DfisanCondAlignedOrUnalignedStoreNFnName[]   = "__dfisan_cond_aligned_or_unaligned_store_id_n";
constexpr char DfisanCondAlignedOrUnalignedStore1FnName[]   = "__dfisan_cond_aligned_or_unaligned_store_id_1";
constexpr char DfisanCondAlignedOrUnalignedStore2FnName[]   = "__dfisan_cond_aligned_or_unaligned_store_id_2";
constexpr char DfisanCondAlignedOrUnalignedStore4FnName[]   = "__dfisan_cond_aligned_or_unaligned_store_id_4";
constexpr char DfisanCondAlignedOrUnalignedStore8FnName[]   = "__dfisan_cond_aligned_or_unaligned_store_id_8";
constexpr char DfisanCondAlignedOrUnalignedStore16FnName[]  = "__dfisan_cond_aligned_or_unaligned_store_id_16";

/// Check function names
constexpr char CommonDfisanLoadFnName[]  = "__dfisan_check_ids";
constexpr char DfisanLoadNFnName[]       = "__dfisan_check_ids_n";
constexpr char DfisanLoad1FnName[]       = "__dfisan_check_ids_1";
constexpr char DfisanLoad2FnName[]       = "__dfisan_check_ids_2";
constexpr char DfisanLoad4FnName[]       = "__dfisan_check_ids_4";
constexpr char DfisanLoad8FnName[]       = "__dfisan_check_ids_8";
constexpr char DfisanLoad16FnName[]      = "__dfisan_check_ids_16";
// aligned
constexpr char DfisanAlignedLoadNFnName[]                 = "__dfisan_aligned_check_ids_n";
constexpr char DfisanAlignedLoad1FnName[]                 = "__dfisan_aligned_check_ids_1";
constexpr char DfisanAlignedLoad2FnName[]                 = "__dfisan_aligned_check_ids_2";
constexpr char DfisanAlignedLoad4FnName[]                 = "__dfisan_aligned_check_ids_4";
constexpr char DfisanAlignedLoad8FnName[]                 = "__dfisan_aligned_check_ids_8";
constexpr char DfisanAlignedLoad16FnName[]                = "__dfisan_aligned_check_ids_16";
// unaligned
constexpr char DfisanUnalignedLoadNFnName[]               = "__dfisan_unaligned_check_ids_n";
constexpr char DfisanUnalignedLoad1FnName[]               = "__dfisan_unaligned_check_ids_1";
constexpr char DfisanUnalignedLoad2FnName[]               = "__dfisan_unaligned_check_ids_2";
constexpr char DfisanUnalignedLoad4FnName[]               = "__dfisan_unaligned_check_ids_4";
constexpr char DfisanUnalignedLoad8FnName[]               = "__dfisan_unaligned_check_ids_8";
constexpr char DfisanUnalignedLoad16FnName[]              = "__dfisan_unaligned_check_ids_16";
// aligned or unaligned
constexpr char DfisanAlignedOrUnalignedLoadNFnName[]      = "__dfisan_aligned_or_unaligned_check_ids_n";
constexpr char DfisanAlignedOrUnalignedLoad1FnName[]      = "__dfisan_aligned_or_unaligned_check_ids_1";
constexpr char DfisanAlignedOrUnalignedLoad2FnName[]      = "__dfisan_aligned_or_unaligned_check_ids_2";
constexpr char DfisanAlignedOrUnalignedLoad4FnName[]      = "__dfisan_aligned_or_unaligned_check_ids_4";
constexpr char DfisanAlignedOrUnalignedLoad8FnName[]      = "__dfisan_aligned_or_unaligned_check_ids_8";
constexpr char DfisanAlignedOrUnalignedLoad16FnName[]     = "__dfisan_aligned_or_unaligned_check_ids_16";
// conditional aligned (for aligned or no-target def)
constexpr char DfisanCondAlignedLoadNFnName[]             = "__dfisan_cond_aligned_check_ids_n";
constexpr char DfisanCondAlignedLoad1FnName[]             = "__dfisan_cond_aligned_check_ids_1";
constexpr char DfisanCondAlignedLoad2FnName[]             = "__dfisan_cond_aligned_check_ids_2";
constexpr char DfisanCondAlignedLoad4FnName[]             = "__dfisan_cond_aligned_check_ids_4";
constexpr char DfisanCondAlignedLoad8FnName[]             = "__dfisan_cond_aligned_check_ids_8";
constexpr char DfisanCondAlignedLoad16FnName[]            = "__dfisan_cond_aligned_check_ids_16";
// conditional unaligned (for unaligned or no-target def)
constexpr char DfisanCondUnalignedLoadNFnName[]           = "__dfisan_cond_unaligned_check_ids_n";
constexpr char DfisanCondUnalignedLoad1FnName[]           = "__dfisan_cond_unaligned_check_ids_1";
constexpr char DfisanCondUnalignedLoad2FnName[]           = "__dfisan_cond_unaligned_check_ids_2";
constexpr char DfisanCondUnalignedLoad4FnName[]           = "__dfisan_cond_unaligned_check_ids_4";
constexpr char DfisanCondUnalignedLoad8FnName[]           = "__dfisan_cond_unaligned_check_ids_8";
constexpr char DfisanCondUnalignedLoad16FnName[]          = "__dfisan_cond_unaligned_check_ids_16";
// conditional aligned or unaligned (for aligned or unaligned or no-target def)
constexpr char DfisanCondAlignedOrUnalignedLoadNFnName[]  = "__dfisan_cond_aligned_or_unaligned_check_ids_n";
constexpr char DfisanCondAlignedOrUnalignedLoad1FnName[]  = "__dfisan_cond_aligned_or_unaligned_check_ids_1";
constexpr char DfisanCondAlignedOrUnalignedLoad2FnName[]  = "__dfisan_cond_aligned_or_unaligned_check_ids_2";
constexpr char DfisanCondAlignedOrUnalignedLoad4FnName[]  = "__dfisan_cond_aligned_or_unaligned_check_ids_4";
constexpr char DfisanCondAlignedOrUnalignedLoad8FnName[]  = "__dfisan_cond_aligned_or_unaligned_check_ids_8";
constexpr char DfisanCondAlignedOrUnalignedLoad16FnName[] = "__dfisan_cond_aligned_or_unaligned_check_ids_16";

/// Unsafe access check
constexpr char DfisanCheckUnsafeAccessFnName[] = "__dfisan_check_unsafe_access";

/// Error report functions
constexpr char DfisanInvalidSafeAccessReportFnName[] = "__dfisan_invalid_safe_access_report";
constexpr char DfisanInvalidUseReportFnName[]        = "__dfisan_invalid_use_report";

/// Memory mapping addresses
constexpr uintptr_t kUnsafeStackEnd     = 0x7fffffffffff;
constexpr uintptr_t kUnsafeStackBeg     = 0x70007fff8000;
constexpr uintptr_t kUnsafeHeapEnd      = 0x10007fffffff;
constexpr uintptr_t kUnsafeHeapBeg      = 0x100000000000;
constexpr uintptr_t kSafeAlignedEnd     = 0x87fffffff;
constexpr uintptr_t kSafeAlignedBeg     = 0x800000000;
  constexpr uintptr_t kSafeAlignedHeapEnd   = kSafeAlignedEnd;
  constexpr uintptr_t kSafeAlignedHeapBeg   = 0x810000000;
  constexpr uintptr_t kSafeAlignedGlobalEnd = 0x80fffffff;
  constexpr uintptr_t kSafeAlignedGlobalBeg = kSafeAlignedBeg;
constexpr uintptr_t kShadowAlignedEnd   = 0x43fffffff;
constexpr uintptr_t kShadowAlignedBeg   = 0x400000000;
constexpr uintptr_t kShadowGapEnd       = 0x3ffffffff;
constexpr uintptr_t kShadowGapBeg       = 0x200000000;
constexpr uintptr_t kShadowUnalignedEnd = 0x1ffffffff;
constexpr uintptr_t kShadowUnalignedBeg = 0x100000000;
constexpr uintptr_t kSafeUnalignedEnd   = 0xffffffff;
constexpr uintptr_t kSafeUnalignedBeg   = 0x80000000;
  constexpr uintptr_t kSafeUnalignedHeapEnd   = kSafeUnalignedEnd;
  constexpr uintptr_t kSafeUnalignedHeapBeg   = 0x90000000;
  constexpr uintptr_t kSafeUnalignedGlobalEnd = 0x8fffffff;
  constexpr uintptr_t kSafeUnalignedGlobalBeg = kSafeUnalignedBeg;
constexpr uintptr_t kLowUnsafeEnd       = 0x7fff7fff;
constexpr uintptr_t kLowUnsafeBeg       = 0x0;

///
//  Command-line flags
///

static cl::opt<bool> ClCheckAllUnsafeAccess(
  "check-all-unsafe-access", cl::desc("Instrument all unsafe accesses to protect targets"),
  cl::Hidden, cl::init(false));

///
//  Statistics
///

STATISTIC(NumInstrumentedUnsafeAccesses,               "Number of instrumented unsafe accesses");
STATISTIC(NumInstrumentedAlignedStores,                "Number of instrumented aligned stores");
STATISTIC(NumInstrumentedUnalignedStores,              "Number of instrumented unaligned stores");
STATISTIC(NumInstrumentedAlignedOrUnalignedStores,     "Number of instrumented aligned-or-unaligned stores");
STATISTIC(NumInstrumentedCondAlignedStores,            "Number of instrumented conditional aligned stores");
STATISTIC(NumInstrumentedCondUnalignedStores,          "Number of instrumented conditional unaligned stores");
STATISTIC(NumInstrumentedCondAlignedOrUnalignedStores, "Number of instrumented conditional aligned or unaligned stores");
STATISTIC(NumInstrumentedAlignedLoads,                 "Number of instrumented aligned loads");
STATISTIC(NumInstrumentedUnalignedLoads,               "Number of instrumented unaligned loads");
STATISTIC(NumInstrumentedAlignedOrUnalignedLoads,      "Number of instrumented aligned-or-unaligned loads");
STATISTIC(NumInstrumentedCondAlignedLoads,             "Number of instrumented conditional aligned loads");
STATISTIC(NumInstrumentedCondUnalignedLoads,           "Number of instrumented conditional unaligned loads");
STATISTIC(NumInstrumentedCondAlignedOrUnalignedLoads,  "Number of instrumented conditional aligned or unaligned loads");

///
//  Runtime check functions
///

static Value *createAddrIsInSafeRegion(IRBuilder<> *IRB, Value *Addr) {
  Type *AddrTy = IRB->getInt64Ty();
  Value *AddrLong = IRB->CreatePtrToInt(Addr, AddrTy);
  Value *SafeBeg  = ConstantInt::get(AddrTy, kSafeUnalignedBeg);
  Value *SafeEnd  = ConstantInt::get(AddrTy, kSafeAlignedEnd);
  Value *Cmp1 = IRB->CreateICmpULE(SafeBeg, AddrLong);  // SafeBeg <= Addr
  Value *Cmp2 = IRB->CreateICmpULE(AddrLong, SafeEnd);  // Addr <= SafeEnd
  Value *Ret  = IRB->CreateLogicalAnd(Cmp1, Cmp2);
  return Ret;
}

Instruction *DataFlowIntegritySanitizerPass::generateCrashCode(Instruction *InsertBefore, Value *Addr, bool IsUnsafe) {
  Builder->SetInsertPoint(InsertBefore);
  Value *AddrLong = Builder->CreatePtrToInt(Addr, Int64Ty);
  CallInst *Call = nullptr;
  if (IsUnsafe) {
    Call = Builder->CreateCall(InvalidSafeAccessReportFn, {AddrLong});
  } else {
    // Call = Builder->CreateCall();
    llvm_unreachable("TODO: Implement generateCrashCode of invalid use");
  }
  return Call;
}

void DataFlowIntegritySanitizerPass::instrumentUnsafeAccess(Instruction *OrigInst, Value *Addr) {
  Builder->SetInsertPoint(OrigInst);
  Value *Cmp = createAddrIsInSafeRegion(Builder.get(), Addr);
  Instruction *CrashTerm = SplitBlockAndInsertIfThen(Cmp, OrigInst, /* unreachable */ true);
  Instruction *Crash = generateCrashCode(CrashTerm, Addr, /* IsUnsafe */ true);
  Crash->setDebugLoc(OrigInst->getDebugLoc());

  NumInstrumentedUnsafeAccesses++;
}

void DataFlowIntegritySanitizerPass::instrumentFunction(Function &Func) {
  using UnsafeInstMap = std::unordered_map<Instruction *, ValueVector>;
  UnsafeInstMap UnsafeMap;
  // Collect unsafe instructions
  for (auto &Inst : instructions(&Func)) {
    getUnsafeAccessTargets(&Inst, UnsafeMap[&Inst]);
  }
  // Instrument unsafe instructions
  for (auto &Iter : UnsafeMap) {
    Instruction *UnsafeInst = Iter.first;
    ValueVector &Targets = Iter.second;
    if (!Targets.empty())
      insertCheckUnsafeAccessFn(UnsafeInst, Targets);
  }
}

///
//  Instrumentation Pass
///

PreservedAnalyses
DataFlowIntegritySanitizerPass::run(Module &M, ModuleAnalysisManager &MAM) {
  LLVM_DEBUG(dbgs() << "DataFlowIntegritySanitizerPass: Insert check functions to enforce data flow integrity\n");

  this->M = &M;
  Builder = std::make_unique<IRBuilder<>>(M.getContext());

  initializeSanitizerFuncs();
  insertDfiInitFn();

  auto &Result = MAM.getResult<UseDefAnalysisPass>(M);
  ProtectInfo = Result.getProtectInfo();
  Opts = Result.getOptions();

  // Instrument unsafe access
  for (auto &Func : M.getFunctionList()) {
    instrumentFunction(Func);
  }

  if (Result.emptyResult()) // skip use-def instrumentation
    return PreservedAnalyses::all();

  DG = Result.getDG();
  DDA = Result.getDDA();

  // Instrument store functions.
  for (auto *AlignedOnlyDef : ProtectInfo->AlignedOnlyDefs) {
    insertDfiStoreFn(AlignedOnlyDef, UseDefKind::Aligned);
  }
  for (auto *UnalignedOnlyDef : ProtectInfo->UnalignedOnlyDefs) {
    insertDfiStoreFn(UnalignedOnlyDef, UseDefKind::Unaligned);
  }
  for (auto *BothOnlyDef : ProtectInfo->BothOnlyDefs) {
    insertDfiStoreFn(BothOnlyDef, UseDefKind::AlignedOrUnaligned);
  }
  for (auto *AlignedOrNoTargetDef : ProtectInfo->AlignedOrNoTargetDefs) {
    insertDfiStoreFn(AlignedOrNoTargetDef, UseDefKind::CondAligned);
  }
  for (auto *UnalignedOrNoTargetDef : ProtectInfo->UnalignedOrNoTargetDefs) {
    insertDfiStoreFn(UnalignedOrNoTargetDef, UseDefKind::CondUnaligned);
  }
  for (auto *BothOrNoTargetDef : ProtectInfo->BothOrNoTargetDefs) {
    insertDfiStoreFn(BothOrNoTargetDef, UseDefKind::CondAlignedOrUnaligned);
  }

  // Instrument check functions.
  for (auto *AlignedOnlyUse : ProtectInfo->AlignedOnlyUses) {
    insertDfiLoadFn(AlignedOnlyUse, UseDefKind::Aligned);
  }
  for (auto *UnalignedOnlyUse : ProtectInfo->UnalignedOnlyUses) {
    insertDfiLoadFn(UnalignedOnlyUse, UseDefKind::Unaligned);
  }
  for (auto *BothOnlyUse : ProtectInfo->BothOnlyUses) {
    insertDfiLoadFn(BothOnlyUse, UseDefKind::AlignedOrUnaligned);
  }
  for (auto *AlignedOrNoTargetUse : ProtectInfo->AlignedOrNoTargetUses) {
    insertDfiLoadFn(AlignedOrNoTargetUse, UseDefKind::CondAligned);
  }
  for (auto *UnalignedOrNoTargetUse : ProtectInfo->UnalignedOrNoTargetUses) {
    insertDfiLoadFn(UnalignedOrNoTargetUse, UseDefKind::CondUnaligned);
  }
  for (auto *BothOrNoTargetUse : ProtectInfo->BothOrNoTargetUses) {
    insertDfiLoadFn(BothOrNoTargetUse, UseDefKind::CondAlignedOrUnaligned);
  }

  return PreservedAnalyses::none();
}

void DataFlowIntegritySanitizerPass::initializeSanitizerFuncs() {
  LLVMContext &Ctx = M->getContext();

  int LongSize = M->getDataLayout().getPointerSizeInBits();
  VoidTy  = Type::getVoidTy(Ctx);
  Int8Ty  = Type::getInt8Ty(Ctx);
  Int16Ty = Type::getInt16Ty(Ctx);
  Int32Ty = Type::getInt32Ty(Ctx);
  Int64Ty = Type::getInt64Ty(Ctx);
  PtrTy   = Type::getIntNTy(Ctx, LongSize);
  Int8PtrTy = Type::getInt8PtrTy(Ctx);

  SmallVector<Type *, 8> StoreArgTypes{PtrTy, Int16Ty};
  FunctionType *StoreFnTy = FunctionType::get(VoidTy, StoreArgTypes, false);
  SmallVector<Type *, 8> StoreNArgTypes{PtrTy, Int64Ty, Int16Ty};
  FunctionType *StoreNFnTy = FunctionType::get(VoidTy, StoreNArgTypes, false);
  SmallVector<Type *, 8> LoadArgTypes{PtrTy, Int32Ty};
  FunctionType *LoadFnTy = FunctionType::get(VoidTy, LoadArgTypes, true);
  SmallVector<Type *, 8> LoadNArgTypes{PtrTy, Int64Ty, Int32Ty};
  FunctionType *LoadNFnTy = FunctionType::get(VoidTy, LoadNArgTypes, true);
  FunctionType *CheckUnsafeAcessFnTy = FunctionType::get(VoidTy, {Int64Ty}, false);
  FunctionType *InvalidSafeAccessReportFnTy = FunctionType::get(VoidTy, {Int64Ty}, false);

  DfiInitFn  = M->getOrInsertFunction(DfisanInitFnName, VoidTy);
  DfiStoreNFn = M->getOrInsertFunction(DfisanStoreNFnName, StoreNFnTy);
  DfiLoadNFn  = M->getOrInsertFunction(DfisanLoadNFnName, LoadNFnTy); // VarArg Function

  DfiStore1Fn = M->getOrInsertFunction(DfisanStore1FnName, StoreFnTy);
  DfiStore2Fn = M->getOrInsertFunction(DfisanStore2FnName, StoreFnTy);
  DfiStore4Fn = M->getOrInsertFunction(DfisanStore4FnName, StoreFnTy);
  DfiStore8Fn = M->getOrInsertFunction(DfisanStore8FnName, StoreFnTy);
  DfiStore16Fn = M->getOrInsertFunction(DfisanStore16FnName, StoreFnTy);

  DfiLoad1Fn = M->getOrInsertFunction(DfisanLoad1FnName, LoadFnTy);
  DfiLoad2Fn = M->getOrInsertFunction(DfisanLoad2FnName, LoadFnTy);
  DfiLoad4Fn = M->getOrInsertFunction(DfisanLoad4FnName, LoadFnTy);
  DfiLoad8Fn = M->getOrInsertFunction(DfisanLoad8FnName, LoadFnTy);
  DfiLoad16Fn = M->getOrInsertFunction(DfisanLoad16FnName, LoadFnTy);

  /// Store functions
  // aligned
  AlignedStoreNFn = M->getOrInsertFunction(DfisanAlignedStoreNFnName, StoreNFnTy);
  AlignedStore1Fn = M->getOrInsertFunction(DfisanAlignedStore1FnName, StoreFnTy);
  AlignedStore2Fn = M->getOrInsertFunction(DfisanAlignedStore2FnName, StoreFnTy);
  AlignedStore4Fn = M->getOrInsertFunction(DfisanAlignedStore4FnName, StoreFnTy);
  AlignedStore8Fn = M->getOrInsertFunction(DfisanAlignedStore8FnName, StoreFnTy);
  AlignedStore16Fn = M->getOrInsertFunction(DfisanAlignedStore16FnName, StoreFnTy);
  // unaligned
  UnalignedStoreNFn = M->getOrInsertFunction(DfisanUnalignedStoreNFnName, StoreNFnTy);
  UnalignedStore1Fn = M->getOrInsertFunction(DfisanUnalignedStore1FnName, StoreFnTy);
  UnalignedStore2Fn = M->getOrInsertFunction(DfisanUnalignedStore2FnName, StoreFnTy);
  UnalignedStore4Fn = M->getOrInsertFunction(DfisanUnalignedStore4FnName, StoreFnTy);
  UnalignedStore8Fn = M->getOrInsertFunction(DfisanUnalignedStore8FnName, StoreFnTy);
  UnalignedStore16Fn = M->getOrInsertFunction(DfisanUnalignedStore16FnName, StoreFnTy);
  // aligned or unaligned
  AlignedOrUnalignedStoreNFn = M->getOrInsertFunction(DfisanAlignedOrUnalignedStoreNFnName, StoreNFnTy);
  AlignedOrUnalignedStore1Fn = M->getOrInsertFunction(DfisanAlignedOrUnalignedStore1FnName, StoreFnTy);
  AlignedOrUnalignedStore2Fn = M->getOrInsertFunction(DfisanAlignedOrUnalignedStore2FnName, StoreFnTy);
  AlignedOrUnalignedStore4Fn = M->getOrInsertFunction(DfisanAlignedOrUnalignedStore4FnName, StoreFnTy);
  AlignedOrUnalignedStore8Fn = M->getOrInsertFunction(DfisanAlignedOrUnalignedStore8FnName, StoreFnTy);
  AlignedOrUnalignedStore16Fn = M->getOrInsertFunction(DfisanAlignedOrUnalignedStore16FnName, StoreFnTy);
  // conditional aligned
  CondAlignedStoreNFn = M->getOrInsertFunction(DfisanCondAlignedStoreNFnName, StoreNFnTy);
  CondAlignedStore1Fn = M->getOrInsertFunction(DfisanCondAlignedStore1FnName, StoreFnTy);
  CondAlignedStore2Fn = M->getOrInsertFunction(DfisanCondAlignedStore2FnName, StoreFnTy);
  CondAlignedStore4Fn = M->getOrInsertFunction(DfisanCondAlignedStore4FnName, StoreFnTy);
  CondAlignedStore8Fn = M->getOrInsertFunction(DfisanCondAlignedStore8FnName, StoreFnTy);
  CondAlignedStore16Fn = M->getOrInsertFunction(DfisanCondAlignedStore16FnName, StoreFnTy);
  // conditional unaligned
  CondUnalignedStoreNFn = M->getOrInsertFunction(DfisanCondUnalignedStoreNFnName, StoreNFnTy);
  CondUnalignedStore1Fn = M->getOrInsertFunction(DfisanCondUnalignedStore1FnName, StoreFnTy);
  CondUnalignedStore2Fn = M->getOrInsertFunction(DfisanCondUnalignedStore2FnName, StoreFnTy);
  CondUnalignedStore4Fn = M->getOrInsertFunction(DfisanCondUnalignedStore4FnName, StoreFnTy);
  CondUnalignedStore8Fn = M->getOrInsertFunction(DfisanCondUnalignedStore8FnName, StoreFnTy);
  CondUnalignedStore16Fn = M->getOrInsertFunction(DfisanCondUnalignedStore16FnName, StoreFnTy);
  // conditional aligned or unaligned
  CondAlignedOrUnalignedStoreNFn = M->getOrInsertFunction(DfisanCondAlignedOrUnalignedStoreNFnName, StoreNFnTy);
  CondAlignedOrUnalignedStore1Fn = M->getOrInsertFunction(DfisanCondAlignedOrUnalignedStore1FnName, StoreFnTy);
  CondAlignedOrUnalignedStore2Fn = M->getOrInsertFunction(DfisanCondAlignedOrUnalignedStore2FnName, StoreFnTy);
  CondAlignedOrUnalignedStore4Fn = M->getOrInsertFunction(DfisanCondAlignedOrUnalignedStore4FnName, StoreFnTy);
  CondAlignedOrUnalignedStore8Fn = M->getOrInsertFunction(DfisanCondAlignedOrUnalignedStore8FnName, StoreFnTy);
  CondAlignedOrUnalignedStore16Fn = M->getOrInsertFunction(DfisanCondAlignedOrUnalignedStore16FnName, StoreFnTy);

  // Load functions
  // aligned
  AlignedLoadNFn = M->getOrInsertFunction(DfisanAlignedLoadNFnName, LoadNFnTy);
  AlignedLoad1Fn = M->getOrInsertFunction(DfisanAlignedLoad1FnName, LoadFnTy);
  AlignedLoad2Fn = M->getOrInsertFunction(DfisanAlignedLoad2FnName, LoadFnTy);
  AlignedLoad4Fn = M->getOrInsertFunction(DfisanAlignedLoad4FnName, LoadFnTy);
  AlignedLoad8Fn = M->getOrInsertFunction(DfisanAlignedLoad8FnName, LoadFnTy);
  AlignedLoad16Fn = M->getOrInsertFunction(DfisanAlignedLoad16FnName, LoadFnTy);
  // unaligned
  UnalignedLoadNFn = M->getOrInsertFunction(DfisanUnalignedLoadNFnName, LoadNFnTy);
  UnalignedLoad1Fn = M->getOrInsertFunction(DfisanUnalignedLoad1FnName, LoadFnTy);
  UnalignedLoad2Fn = M->getOrInsertFunction(DfisanUnalignedLoad2FnName, LoadFnTy);
  UnalignedLoad4Fn = M->getOrInsertFunction(DfisanUnalignedLoad4FnName, LoadFnTy);
  UnalignedLoad8Fn = M->getOrInsertFunction(DfisanUnalignedLoad8FnName, LoadFnTy);
  UnalignedLoad16Fn = M->getOrInsertFunction(DfisanUnalignedLoad16FnName, LoadFnTy);
  // aligned or unaligned
  AlignedOrUnalignedLoadNFn = M->getOrInsertFunction(DfisanAlignedOrUnalignedLoadNFnName, LoadNFnTy);
  AlignedOrUnalignedLoad1Fn = M->getOrInsertFunction(DfisanAlignedOrUnalignedLoad1FnName, LoadFnTy);
  AlignedOrUnalignedLoad2Fn = M->getOrInsertFunction(DfisanAlignedOrUnalignedLoad2FnName, LoadFnTy);
  AlignedOrUnalignedLoad4Fn = M->getOrInsertFunction(DfisanAlignedOrUnalignedLoad4FnName, LoadFnTy);
  AlignedOrUnalignedLoad8Fn = M->getOrInsertFunction(DfisanAlignedOrUnalignedLoad8FnName, LoadFnTy);
  AlignedOrUnalignedLoad16Fn = M->getOrInsertFunction(DfisanAlignedOrUnalignedLoad16FnName, LoadFnTy);
  // conditional aligned
  CondAlignedLoadNFn = M->getOrInsertFunction(DfisanCondAlignedLoadNFnName, LoadNFnTy);
  CondAlignedLoad1Fn = M->getOrInsertFunction(DfisanCondAlignedLoad1FnName, LoadFnTy);
  CondAlignedLoad2Fn = M->getOrInsertFunction(DfisanCondAlignedLoad2FnName, LoadFnTy);
  CondAlignedLoad4Fn = M->getOrInsertFunction(DfisanCondAlignedLoad4FnName, LoadFnTy);
  CondAlignedLoad8Fn = M->getOrInsertFunction(DfisanCondAlignedLoad8FnName, LoadFnTy);
  CondAlignedLoad16Fn = M->getOrInsertFunction(DfisanCondAlignedLoad16FnName, LoadFnTy);
  // conditional unaligned
  CondUnalignedLoadNFn = M->getOrInsertFunction(DfisanCondUnalignedLoadNFnName, LoadNFnTy);
  CondUnalignedLoad1Fn = M->getOrInsertFunction(DfisanCondUnalignedLoad1FnName, LoadFnTy);
  CondUnalignedLoad2Fn = M->getOrInsertFunction(DfisanCondUnalignedLoad2FnName, LoadFnTy);
  CondUnalignedLoad4Fn = M->getOrInsertFunction(DfisanCondUnalignedLoad4FnName, LoadFnTy);
  CondUnalignedLoad8Fn = M->getOrInsertFunction(DfisanCondUnalignedLoad8FnName, LoadFnTy);
  CondUnalignedLoad16Fn = M->getOrInsertFunction(DfisanCondUnalignedLoad16FnName, LoadFnTy);
  // conditional aligned or unaligned
  CondAlignedOrUnalignedLoadNFn = M->getOrInsertFunction(DfisanCondAlignedOrUnalignedLoadNFnName, LoadNFnTy);
  CondAlignedOrUnalignedLoad1Fn = M->getOrInsertFunction(DfisanCondAlignedOrUnalignedLoad1FnName, LoadFnTy);
  CondAlignedOrUnalignedLoad2Fn = M->getOrInsertFunction(DfisanCondAlignedOrUnalignedLoad2FnName, LoadFnTy);
  CondAlignedOrUnalignedLoad4Fn = M->getOrInsertFunction(DfisanCondAlignedOrUnalignedLoad4FnName, LoadFnTy);
  CondAlignedOrUnalignedLoad8Fn = M->getOrInsertFunction(DfisanCondAlignedOrUnalignedLoad8FnName, LoadFnTy);
  CondAlignedOrUnalignedLoad16Fn = M->getOrInsertFunction(DfisanCondAlignedOrUnalignedLoad16FnName, LoadFnTy);

  // Unsafe access check
  CheckUnsafeAccessFn = M->getOrInsertFunction(DfisanCheckUnsafeAccessFnName, CheckUnsafeAcessFnTy);

  // Error report
  InvalidSafeAccessReportFn = M->getOrInsertFunction(DfisanInvalidSafeAccessReportFnName, InvalidSafeAccessReportFnTy);
}

/// Insert a constructor function in comdat
void DataFlowIntegritySanitizerPass::insertDfiInitFn() {
  LLVM_DEBUG(dbgs() << __func__ << "\n");

  // Create Sanitizer Ctor
  Ctor = Function::createWithDefaultAttr(
    FunctionType::get(VoidTy, false),
    GlobalValue::InternalLinkage, 0, DfisanModuleCtorName, M);
  Ctor->addFnAttr(Attribute::NoUnwind);
  BasicBlock *CtorBB = BasicBlock::Create(M->getContext(), "", Ctor);
  ReturnInst::Create(M->getContext(), CtorBB);

  // Insert Init Function
  Builder->SetInsertPoint(Ctor->getEntryBlock().getTerminator());
  Builder->CreateCall(DfiInitFn, {});

  // Ensure Ctor cannot be discarded, even if in a comdat.
  appendToUsed(*M, {Ctor});
  // Put the constructor in GlobalCtors
  appendToGlobalCtors(*M, Ctor, 1);

  return;
}

/// Insert a DEF function after each store statement using pointer.
void DataFlowIntegritySanitizerPass::insertDfiStoreFn(Value *Def, UseDefKind Kind) {
  LLVM_DEBUG(llvm::dbgs() << "InsertDfiStoreFn: " << *Def << "\n");

  if (Instruction *DefInst = dyn_cast<Instruction>(Def)) {
    if (StoreInst *Store = dyn_cast<StoreInst>(DefInst)) {
      auto *StoreTarget = Store->getPointerOperand();
      unsigned Size = M->getDataLayout().getTypeStoreSize(StoreTarget->getType()->getNonOpaquePointerElementType());
      createDfiStoreFn(ProtectInfo->getDefID(Store), StoreTarget, Size, Kind, Store->getNextNode());
    }
    else if (CallInst *Call = dyn_cast<CallInst>(DefInst)) {
      auto *Callee = Call->getCalledFunction();
      auto FnName = Callee->getName().str();
      if (Opts->getAllocationFunction(Callee->getName().str()) == dg::AllocationFunction::CALLOC) { // Handling of calloc
        auto *Nmem = Call->getOperand(0);
        auto *Size = Call->getOperand(1);
        Builder->SetInsertPoint(Call->getNextNode());
        auto *SizeVal = Builder->CreateNUWMul(Nmem, Size);
        createDfiStoreFn(ProtectInfo->getDefID(Call), Def, SizeVal, Kind);
      } else if (const dg::FunctionModel *Model = Opts->getFunctionModel(FnName)) { // Handling of stdlib functions
        for (const auto &Iter : Model->getDefines()) {
          auto &Ope = Iter.second;
          unsigned OpeNum = Ope.operand;
          if (OpeNum == dg::VARARG) {
            assert(Ope.from.isOperand() && "VARARG's from must be operand");
            unsigned FromOpe = Ope.from.getOperand();
            for (unsigned Idx = FromOpe; Idx < Call->arg_size(); Idx++) {
              auto *StoreTarget = Call->getOperand(Idx);
              unsigned Size = M->getDataLayout().getTypeStoreSize(StoreTarget->getType()->getNonOpaquePointerElementType());
              createDfiStoreFn(ProtectInfo->getDefID(Call), StoreTarget, Size, Kind, Call->getNextNode());
            }
          } else {
            auto *StoreTarget = (OpeNum == dg::RETURN) ? Call : Call->getOperand(OpeNum);
            if (Ope.to.isOperand()) {
              auto *SizeVal = Call->getOperand(Ope.to.getOperand());
              createDfiStoreFn(ProtectInfo->getDefID(Call), StoreTarget, SizeVal, Kind, Call->getNextNode());
            } else if (Ope.to.isOffset()) {
              unsigned Size = Ope.to.getOffset().offset -  Ope.from.getOffset().offset + 1;
              createDfiStoreFn(ProtectInfo->getDefID(Call), StoreTarget, Size, Kind, Call->getNextNode());
            }
          }
        }
      } else {
        llvm::errs() << "TODO: Support Def-stdlib function: " << FnName << "\n";
      }
    }
    else if (AllocaInst *Alloca = dyn_cast<AllocaInst>(Def)) {
      // Do nothing
    } else {
      llvm::errs() << "No support DefInst: " << *DefInst << "\n";
      // llvm_unreachable("No support DefInst");
    }
  } else if (GlobalVariable *GlobVar = dyn_cast<GlobalVariable>(Def)) {
    unsigned Size = M->getDataLayout().getTypeStoreSize(GlobVar->getType()->getNonOpaquePointerElementType());
    createDfiStoreFn(ProtectInfo->getDefID(GlobVar), GlobVar, Size, Kind, Ctor->getEntryBlock().getTerminator());
    insertGlobalInit(GlobVar);
  } else {
    llvm::errs() << "No support Def: " << *Def << "\n";
    // llvm_unreachable("No support Def");
  }
}

/// Insert a CHECK function before each load statement.
void DataFlowIntegritySanitizerPass::insertDfiLoadFn(Instruction *Use, UseDefKind Kind) {
  LLVM_DEBUG(llvm::dbgs() << "InsertDfiLoadFn: " << *Use << "\n");

  // Skip unknown value uses (e.g., argv[]).
  const auto &UseSites = DDA->getNode(Use)->getUses();
  if (UseSites.size() == 1 && DDA->getValue(UseSites.begin()->target) == nullptr) {
    llvm::errs() << "Skip use: " << *Use << "\n";
    return;
  }

  // collect def-id's
  std::set<dg::DefID> DefIDSet;   // Use std::set to prevent duplicate
  for (auto *Def : ProtectInfo->UseDef[Use]) {
    if (ProtectInfo->hasDefID(Def)) // Def may be a no-target instruction.
      DefIDSet.insert(ProtectInfo->getDefID(Def));
  }

  // create vector from DefIDSet
  ValueVector DefIDs;
  for (auto DefID : DefIDSet) {
    Value *IDVal = ConstantInt::get(Int32Ty, DefID, false);
    DefIDs.push_back(IDVal);
  }

  if (LoadInst *Load = dyn_cast<LoadInst>(Use)) {
    auto *LoadTarget = Load->getPointerOperand();
    unsigned Size = M->getDataLayout().getTypeStoreSize(LoadTarget->getType()->getNonOpaquePointerElementType());
    createDfiLoadFn(LoadTarget, Size, DefIDs, Kind, Load);
  } else if (CallInst *Call = dyn_cast<CallInst>(Use)) {  // Handling of stdlib functions
    auto *Callee = Call->getCalledFunction();
    auto FnName = Callee->getName().str();
    if (const dg::FunctionModel *Model = Opts->getFunctionModel(FnName)) {
      for (const auto &Iter : Model->getUses()) {
        auto &Ope = Iter.second;
        unsigned OpeNum = Ope.operand;
        if (OpeNum == dg::VARARG) {
          llvm::errs() << "TODO: Support VARARG use: " << FnName << "\n";
        } else {
          llvm::errs() << "TODO: Support Use-stdlib function: " << FnName << "\n";
        }
      }
    }
  } else {
    llvm::errs() << "No support Use: " << *Use << "\n";
    // llvm_unreachable("No support Use");
  }
}

/// Create a function call to DfiStoreFn.
void DataFlowIntegritySanitizerPass::createDfiStoreFn(dg::DefID DefID, Value *StoreTarget, unsigned Size, UseDefKind Kind, Instruction *InsertPoint) {
  Value *SizeVal = ConstantInt::get(Int64Ty, Size, false);
  createDfiStoreFn(DefID, StoreTarget, SizeVal, Kind, InsertPoint);
}

void DataFlowIntegritySanitizerPass::createDfiStoreFn(dg::DefID DefID, Value *StoreTarget, Value *SizeVal, UseDefKind Kind, Instruction *InsertPoint) {
  if (InsertPoint != nullptr)
    Builder->SetInsertPoint(InsertPoint);
  unsigned Size = llvm::isa<ConstantInt>(SizeVal) ? llvm::cast<ConstantInt>(SizeVal)->getZExtValue() : 0;
  Value *SizeArg = SizeVal;
  if (SizeVal->getType() != Int64Ty) {
    if (llvm::isa<ConstantInt>(SizeVal))
      SizeArg = ConstantInt::get(Int64Ty, Size);
    else
      SizeArg = Builder->CreateBitCast(SizeVal, Int64Ty);
  }
  Value *StoreAddr = Builder->CreatePtrToInt(StoreTarget, PtrTy);
  Value *DefIDVal = ConstantInt::get(Int16Ty, DefID);
  ValueVector Args {StoreAddr, DefIDVal};
  ValueVector NArgs = {StoreAddr, SizeArg, DefIDVal};

  switch(Kind) {
  case UseDefKind::Aligned: {
    switch(Size) {
    case 1:   Builder->CreateCall(AlignedStore1Fn, Args);  break;
    case 2:   Builder->CreateCall(AlignedStore2Fn, Args);  break;
    case 4:   Builder->CreateCall(AlignedStore4Fn, Args);  break;
    case 8:   Builder->CreateCall(AlignedStore8Fn, Args);  break;
    case 16:  Builder->CreateCall(AlignedStore16Fn, Args); break;
    default:  Builder->CreateCall(AlignedStoreNFn, NArgs); break;
    }
    NumInstrumentedAlignedStores++;
    break;
  }
  case UseDefKind::Unaligned: {
    switch(Size) {
    case 1:   Builder->CreateCall(UnalignedStore1Fn, Args);  break;
    case 2:   Builder->CreateCall(UnalignedStore2Fn, Args);  break;
    case 4:   Builder->CreateCall(UnalignedStore4Fn, Args);  break;
    case 8:   Builder->CreateCall(UnalignedStore8Fn, Args);  break;
    case 16:  Builder->CreateCall(UnalignedStore16Fn, Args); break;
    default:  Builder->CreateCall(UnalignedStoreNFn, NArgs); break;
    }
    NumInstrumentedUnalignedStores++;
    break;
  } 
  case UseDefKind::AlignedOrUnaligned: {
    switch(Size) {
    case 1:   Builder->CreateCall(AlignedOrUnalignedStore1Fn, Args);  break;
    case 2:   Builder->CreateCall(AlignedOrUnalignedStore2Fn, Args);  break;
    case 4:   Builder->CreateCall(AlignedOrUnalignedStore4Fn, Args);  break;
    case 8:   Builder->CreateCall(AlignedOrUnalignedStore8Fn, Args);  break;
    case 16:  Builder->CreateCall(AlignedOrUnalignedStore16Fn, Args); break;
    default:  Builder->CreateCall(AlignedOrUnalignedStoreNFn, NArgs); break;
    }
    NumInstrumentedAlignedOrUnalignedStores++;
    break;
  }
  case UseDefKind::CondAligned: {
    switch(Size) {
    case 1:   Builder->CreateCall(CondAlignedStore1Fn, Args);  break;
    case 2:   Builder->CreateCall(CondAlignedStore2Fn, Args);  break;
    case 4:   Builder->CreateCall(CondAlignedStore4Fn, Args);  break;
    case 8:   Builder->CreateCall(CondAlignedStore8Fn, Args);  break;
    case 16:  Builder->CreateCall(CondAlignedStore16Fn, Args); break;
    default:  Builder->CreateCall(CondAlignedStoreNFn, NArgs); break;
    }
    NumInstrumentedCondAlignedStores++;
    break;
  }
  case UseDefKind::CondUnaligned: {
    switch(Size) {
    case 1:   Builder->CreateCall(CondUnalignedStore1Fn, Args);  break;
    case 2:   Builder->CreateCall(CondUnalignedStore2Fn, Args);  break;
    case 4:   Builder->CreateCall(CondUnalignedStore4Fn, Args);  break;
    case 8:   Builder->CreateCall(CondUnalignedStore8Fn, Args);  break;
    case 16:  Builder->CreateCall(CondUnalignedStore16Fn, Args); break;
    default:  Builder->CreateCall(CondUnalignedStoreNFn, NArgs); break;
    }
    NumInstrumentedCondUnalignedStores++;
    break;
  }
  case UseDefKind::CondAlignedOrUnaligned: {
    switch(Size) {
    case 1:   Builder->CreateCall(CondAlignedOrUnalignedStore1Fn, Args);  break;
    case 2:   Builder->CreateCall(CondAlignedOrUnalignedStore2Fn, Args);  break;
    case 4:   Builder->CreateCall(CondAlignedOrUnalignedStore4Fn, Args);  break;
    case 8:   Builder->CreateCall(CondAlignedOrUnalignedStore8Fn, Args);  break;
    case 16:  Builder->CreateCall(CondAlignedOrUnalignedStore16Fn, Args); break;
    default:  Builder->CreateCall(CondAlignedOrUnalignedStoreNFn, NArgs); break;
    }
    NumInstrumentedCondAlignedOrUnalignedStores++;
    break;
  }
  // default: llvm_unreachable("Invalid UseDefKind");
  }
}


/// Create a function call to DfiLoadFn.
void DataFlowIntegritySanitizerPass::createDfiLoadFn(Value *LoadTarget, unsigned Size, ValueVector &DefIDs, UseDefKind Kind, Instruction *InsertPoint) {
  Value *SizeVal = ConstantInt::get(Int64Ty, Size, false);
  createDfiLoadFn(LoadTarget, SizeVal, DefIDs, Kind, InsertPoint);
}

void DataFlowIntegritySanitizerPass::createDfiLoadFn(Value *LoadTarget, Value *SizeVal, ValueVector &DefIDs, UseDefKind Kind, Instruction *InsertPoint) {
  if (InsertPoint != nullptr)
    Builder->SetInsertPoint(InsertPoint);
  unsigned Size = llvm::isa<ConstantInt>(SizeVal) ? llvm::cast<ConstantInt>(SizeVal)->getZExtValue() : 0;
  Value *SizeArg = SizeVal;
  if (SizeVal->getType() != Int64Ty) {
    if (llvm::isa<ConstantInt>(SizeVal))
      SizeArg = ConstantInt::get(Int64Ty, Size);
    else
      SizeArg = Builder->CreateBitCast(SizeVal, Int64Ty);
  }
  Value *LoadAddr = Builder->CreatePtrToInt(LoadTarget, PtrTy);
  Value *Argc = ConstantInt::get(Int32Ty, DefIDs.size(), false);

  ValueVector Args{LoadAddr, Argc};
  Args.append(DefIDs);
  ValueVector NArgs{LoadAddr, SizeArg, Argc};
  NArgs.append(DefIDs);

  switch(Kind) {
  case UseDefKind::Aligned: {
    switch(Size) {
    case 1:   Builder->CreateCall(AlignedLoad1Fn, Args);  break;
    case 2:   Builder->CreateCall(AlignedLoad2Fn, Args);  break;
    case 4:   Builder->CreateCall(AlignedLoad4Fn, Args);  break;
    case 8:   Builder->CreateCall(AlignedLoad8Fn, Args);  break;
    case 16:  Builder->CreateCall(AlignedLoad16Fn, Args); break;
    default:  Builder->CreateCall(AlignedLoadNFn, NArgs); break;
    }
    NumInstrumentedAlignedLoads++;
    break;
  }
  case UseDefKind::Unaligned: {
    switch(Size) {
    case 1:   Builder->CreateCall(UnalignedLoad1Fn, Args);  break;
    case 2:   Builder->CreateCall(UnalignedLoad2Fn, Args);  break;
    case 4:   Builder->CreateCall(UnalignedLoad4Fn, Args);  break;
    case 8:   Builder->CreateCall(UnalignedLoad8Fn, Args);  break;
    case 16:  Builder->CreateCall(UnalignedLoad16Fn, Args); break;
    default:  Builder->CreateCall(UnalignedLoadNFn, NArgs); break;
    }
    NumInstrumentedUnalignedLoads++;
    break;
  } 
  case UseDefKind::AlignedOrUnaligned: {
    switch(Size) {
    case 1:   Builder->CreateCall(AlignedOrUnalignedLoad1Fn, Args);  break;
    case 2:   Builder->CreateCall(AlignedOrUnalignedLoad2Fn, Args);  break;
    case 4:   Builder->CreateCall(AlignedOrUnalignedLoad4Fn, Args);  break;
    case 8:   Builder->CreateCall(AlignedOrUnalignedLoad8Fn, Args);  break;
    case 16:  Builder->CreateCall(AlignedOrUnalignedLoad16Fn, Args); break;
    default:  Builder->CreateCall(AlignedOrUnalignedLoadNFn, NArgs); break;
    }
    NumInstrumentedAlignedOrUnalignedLoads++;
    break;
  }
  case UseDefKind::CondAligned: {
    switch(Size) {
    case 1:   Builder->CreateCall(CondAlignedLoad1Fn, Args);  break;
    case 2:   Builder->CreateCall(CondAlignedLoad2Fn, Args);  break;
    case 4:   Builder->CreateCall(CondAlignedLoad4Fn, Args);  break;
    case 8:   Builder->CreateCall(CondAlignedLoad8Fn, Args);  break;
    case 16:  Builder->CreateCall(CondAlignedLoad16Fn, Args); break;
    default:  Builder->CreateCall(CondAlignedLoadNFn, NArgs); break;
    }
    NumInstrumentedCondAlignedLoads++;
    break;
  }
  case UseDefKind::CondUnaligned: {
    switch(Size) {
    case 1:   Builder->CreateCall(CondUnalignedLoad1Fn, Args);  break;
    case 2:   Builder->CreateCall(CondUnalignedLoad2Fn, Args);  break;
    case 4:   Builder->CreateCall(CondUnalignedLoad4Fn, Args);  break;
    case 8:   Builder->CreateCall(CondUnalignedLoad8Fn, Args);  break;
    case 16:  Builder->CreateCall(CondUnalignedLoad16Fn, Args); break;
    default:  Builder->CreateCall(CondUnalignedLoadNFn, NArgs); break;
    }
    NumInstrumentedCondUnalignedLoads++;
    break;
  }
  case UseDefKind::CondAlignedOrUnaligned: {
    switch(Size) {
    case 1:   Builder->CreateCall(CondAlignedOrUnalignedLoad1Fn, Args);  break;
    case 2:   Builder->CreateCall(CondAlignedOrUnalignedLoad2Fn, Args);  break;
    case 4:   Builder->CreateCall(CondAlignedOrUnalignedLoad4Fn, Args);  break;
    case 8:   Builder->CreateCall(CondAlignedOrUnalignedLoad8Fn, Args);  break;
    case 16:  Builder->CreateCall(CondAlignedOrUnalignedLoad16Fn, Args); break;
    default:  Builder->CreateCall(CondAlignedOrUnalignedLoadNFn, NArgs); break;
    }
    NumInstrumentedCondAlignedOrUnalignedLoads++;
    break;
  }
  // default: llvm_unreachable("Invalid UseDefKind");
  }
}

inline bool DataFlowIntegritySanitizerPass::isUnsafeAccessTarget(Value *Target) {
  Value *Base = Target->stripInBoundsConstantOffsets(); // strip constant offsets and get base object
  if (ProtectInfo->hasTarget(Base))
    return false;
  if (ClCheckAllUnsafeAccess)
    return true;
  // Do not check these constant offset access.
  if (GlobalVariable *GlobVar = dyn_cast<GlobalVariable>(Base))
    return false;
  if (AllocaInst *Alloca = dyn_cast<AllocaInst>(Base))
    return false;
  return true;
}

inline void DataFlowIntegritySanitizerPass::addIfUnsafeAccessTarget(ValueVector &Targets, Value *Target) {
  if (isUnsafeAccessTarget(Target))
    Targets.push_back(Target);
}

inline void DataFlowIntegritySanitizerPass::getUnsafeAccessTargets(Instruction *Inst, ValueVector &Targets) {
  if (auto *Load = dyn_cast<LoadInst>(Inst)) {
    if (!ProtectInfo->hasUse(Load))
      addIfUnsafeAccessTarget(Targets, Load->getPointerOperand());
  } else if (auto *Store = dyn_cast<StoreInst>(Inst)) {
    if (!ProtectInfo->hasDef(Store))
      addIfUnsafeAccessTarget(Targets, Store->getPointerOperand());
  }
  else if (auto *Call = dyn_cast<CallInst>(Inst)) {
    auto *Callee = Call->getCalledOperand();
    auto FnName = Callee->getName().str();
    if (const auto *Model = Opts->getFunctionModel(FnName)) {
      if (!Model->getUses().empty() && !ProtectInfo->hasUse(Call)) {
        for (const auto &Iter : Model->getUses()) {
          auto &Ope = Iter.second;
          unsigned OpeNum = Ope.operand;
          if (OpeNum == dg::VARARG) {
            assert(Ope.from.isOperand() && "VARARG's from must be operand");
            unsigned FromOpe = Ope.from.getOperand();
            for (unsigned Idx = FromOpe; Idx < Call->arg_size(); Idx++) {
              addIfUnsafeAccessTarget(Targets, Call->getOperand(Idx)->stripPointerCasts());
            }
          } else if (OpeNum == dg::RETURN) {
            addIfUnsafeAccessTarget(Targets, Call);
          } else {
            addIfUnsafeAccessTarget(Targets, Call->getOperand(OpeNum)->stripPointerCasts());
          }
        }
      }
      if (!Model->getDefines().empty() && !ProtectInfo->hasDef(Call)) {
        for (const auto &Iter : Model->getDefines()) {
          auto &Ope = Iter.second;
          unsigned OpeNum = Ope.operand;
          if (OpeNum == dg::VARARG) {
            assert(Ope.from.isOperand() && "VARARG's from must be operand");
            unsigned FromOpe = Ope.from.getOperand();
            for (unsigned Idx = FromOpe; Idx < Call->arg_size(); Idx++)
              addIfUnsafeAccessTarget(Targets, Call->getOperand(Idx)->stripPointerCasts());
          } else if (OpeNum == dg::RETURN) {
            addIfUnsafeAccessTarget(Targets, Call);
          } else {
            addIfUnsafeAccessTarget(Targets, Call->getOperand(OpeNum)->stripPointerCasts());
          }
        }
      }
    }
  }
}

void DataFlowIntegritySanitizerPass::insertCheckUnsafeAccessFn(Instruction *Inst, ValueVector &Targets) {
  Builder->SetInsertPoint(Inst);
  for (auto *Target : Targets) {
    instrumentUnsafeAccess(Inst, Target);
  }
}

// Insert global variable inits for protection targets.
void DataFlowIntegritySanitizerPass::insertGlobalInit(GlobalVariable *GlobVar) {
  if (!GlobVar->hasInitializer())
    return;
  
  Type *ValTy = GlobVar->getValueType();
  Constant *InitVal = GlobVar->getInitializer();
  LLVM_DEBUG(dbgs() << __func__ << " GlobVar: " << *GlobVar << ", InitVal: " << *InitVal << "\n");
  Builder->SetInsertPoint(Ctor->getEntryBlock().getTerminator());
  if (ValTy->isIntOrPtrTy()) {
    Builder->CreateStore(InitVal, GlobVar);
  } else if (ValTy->isAggregateType()) {
    TypeSize Size = M->getDataLayout().getTypeStoreSize(ValTy);
    Value *SizeVal = ConstantInt::get(Int64Ty, Size.getFixedSize());
    Value *CastedGlobVar = Builder->CreateBitCast(GlobVar, Int8PtrTy);
    GlobalVariable *GlobInitVal = new GlobalVariable(*M, InitVal->getType(), true, GlobalValue::PrivateLinkage, InitVal, GlobVar->getName() + "_init");
    Value *CastedGlobInitVal = Builder->CreateBitCast(GlobInitVal, Int8PtrTy);
    Builder->CreateMemCpyInline(CastedGlobVar, GlobVar->getAlign(), CastedGlobInitVal, GlobVar->getAlign(), SizeVal);
  }
}
