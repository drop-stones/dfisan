#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/TypeFinder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Transforms/Instrumentation/DataFlowIntegritySanitizer.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"  /* appendToUsed */
#include "llvm/Transforms/Utils/BasicBlockUtils.h"  /* SplitBlockAndInsertIfThen */
#include "llvm/Support/Debug.h"
#include "llvm/ADT/Statistic.h"

#include "ModulePass/SVFDefUseAnalysisPass.h"
#include "Dfisan/ProtectInfo.h"
#include "Dfisan/DfisanExtAPI.h"

#include <unordered_map>

#define DEBUG_TYPE "dfi-instrument"

using namespace llvm;
using namespace SVF;

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

// Check and set function names
// aligned
constexpr char DfisanAlignedLoadStoreNFnName[]                 = "__dfisan_aligned_check_set_ids_n";
constexpr char DfisanAlignedLoadStore1FnName[]                 = "__dfisan_aligned_check_set_ids_1";
constexpr char DfisanAlignedLoadStore2FnName[]                 = "__dfisan_aligned_check_set_ids_2";
constexpr char DfisanAlignedLoadStore4FnName[]                 = "__dfisan_aligned_check_set_ids_4";
constexpr char DfisanAlignedLoadStore8FnName[]                 = "__dfisan_aligned_check_set_ids_8";
constexpr char DfisanAlignedLoadStore16FnName[]                = "__dfisan_aligned_check_set_ids_16";
// unaligned
constexpr char DfisanUnalignedLoadStoreNFnName[]               = "__dfisan_unaligned_check_set_ids_n";
constexpr char DfisanUnalignedLoadStore1FnName[]               = "__dfisan_unaligned_check_set_ids_1";
constexpr char DfisanUnalignedLoadStore2FnName[]               = "__dfisan_unaligned_check_set_ids_2";
constexpr char DfisanUnalignedLoadStore4FnName[]               = "__dfisan_unaligned_check_set_ids_4";
constexpr char DfisanUnalignedLoadStore8FnName[]               = "__dfisan_unaligned_check_set_ids_8";
constexpr char DfisanUnalignedLoadStore16FnName[]              = "__dfisan_unaligned_check_set_ids_16";
// aligned or unaligned
constexpr char DfisanAlignedOrUnalignedLoadStoreNFnName[]      = "__dfisan_aligned_or_unaligned_check_set_ids_n";
constexpr char DfisanAlignedOrUnalignedLoadStore1FnName[]      = "__dfisan_aligned_or_unaligned_check_set_ids_1";
constexpr char DfisanAlignedOrUnalignedLoadStore2FnName[]      = "__dfisan_aligned_or_unaligned_check_set_ids_2";
constexpr char DfisanAlignedOrUnalignedLoadStore4FnName[]      = "__dfisan_aligned_or_unaligned_check_set_ids_4";
constexpr char DfisanAlignedOrUnalignedLoadStore8FnName[]      = "__dfisan_aligned_or_unaligned_check_set_ids_8";
constexpr char DfisanAlignedOrUnalignedLoadStore16FnName[]     = "__dfisan_aligned_or_unaligned_check_set_ids_16";
// conditional aligned (for aligned or no-target def)
constexpr char DfisanCondAlignedLoadStoreNFnName[]             = "__dfisan_cond_aligned_check_set_ids_n";
constexpr char DfisanCondAlignedLoadStore1FnName[]             = "__dfisan_cond_aligned_check_set_ids_1";
constexpr char DfisanCondAlignedLoadStore2FnName[]             = "__dfisan_cond_aligned_check_set_ids_2";
constexpr char DfisanCondAlignedLoadStore4FnName[]             = "__dfisan_cond_aligned_check_set_ids_4";
constexpr char DfisanCondAlignedLoadStore8FnName[]             = "__dfisan_cond_aligned_check_set_ids_8";
constexpr char DfisanCondAlignedLoadStore16FnName[]            = "__dfisan_cond_aligned_check_set_ids_16";
// conditional unaligned (for unaligned or no-target def)
constexpr char DfisanCondUnalignedLoadStoreNFnName[]           = "__dfisan_cond_unaligned_check_set_ids_n";
constexpr char DfisanCondUnalignedLoadStore1FnName[]           = "__dfisan_cond_unaligned_check_set_ids_1";
constexpr char DfisanCondUnalignedLoadStore2FnName[]           = "__dfisan_cond_unaligned_check_set_ids_2";
constexpr char DfisanCondUnalignedLoadStore4FnName[]           = "__dfisan_cond_unaligned_check_set_ids_4";
constexpr char DfisanCondUnalignedLoadStore8FnName[]           = "__dfisan_cond_unaligned_check_set_ids_8";
constexpr char DfisanCondUnalignedLoadStore16FnName[]          = "__dfisan_cond_unaligned_check_set_ids_16";
// conditional aligned or unaligned (for aligned or unaligned or no-target def)
constexpr char DfisanCondAlignedOrUnalignedLoadStoreNFnName[]  = "__dfisan_cond_aligned_or_unaligned_check_set_ids_n";
constexpr char DfisanCondAlignedOrUnalignedLoadStore1FnName[]  = "__dfisan_cond_aligned_or_unaligned_check_set_ids_1";
constexpr char DfisanCondAlignedOrUnalignedLoadStore2FnName[]  = "__dfisan_cond_aligned_or_unaligned_check_set_ids_2";
constexpr char DfisanCondAlignedOrUnalignedLoadStore4FnName[]  = "__dfisan_cond_aligned_or_unaligned_check_set_ids_4";
constexpr char DfisanCondAlignedOrUnalignedLoadStore8FnName[]  = "__dfisan_cond_aligned_or_unaligned_check_set_ids_8";
constexpr char DfisanCondAlignedOrUnalignedLoadStore16FnName[] = "__dfisan_cond_aligned_or_unaligned_check_set_ids_16";

/// Unsafe access check
constexpr char DfisanCheckUnsafeAccessFnName[] = "__dfisan_check_unsafe_access";

/// Error report functions
constexpr char DfisanInvalidSafeAccessReportFnName[] = "__dfisan_invalid_safe_access_report";
constexpr char DfisanInvalidUseReportFnName[]        = "__dfisan_invalid_use_report";

/// Shift Width
constexpr unsigned kAlignedShiftWidth = 1;
constexpr unsigned kUnalignedShiftWidth = 1;
constexpr unsigned kAlignedMemGranularity = 4;
constexpr unsigned kShadowGranularity = 2;

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

static cl::opt<bool> ClCheckWithCall(
  "check-with-call", cl::desc("Instrument function calls to check data-flow integrity before load and after store"),
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
  Value *AddrLong = (Addr->getType()->isPointerTy()) ? IRB->CreatePtrToInt(Addr, AddrTy) : Addr;
  Value *SafeBeg  = ConstantInt::get(AddrTy, kSafeUnalignedBeg);
  Value *SafeEnd  = ConstantInt::get(AddrTy, kSafeAlignedEnd);
  Value *Cmp1 = IRB->CreateICmpULE(SafeBeg, AddrLong);  // SafeBeg <= Addr
  Value *Cmp2 = IRB->CreateICmpULE(AddrLong, SafeEnd);  // Addr <= SafeEnd
  Value *Ret  = IRB->CreateLogicalAnd(Cmp1, Cmp2);
  return Ret;
}
static Value *createAddrIsInSafeAlignedRegion(IRBuilder<> *IRB, Value *Addr) {
  Type *AddrTy = IRB->getInt64Ty();
  Value *AddrLong = (Addr->getType()->isPointerTy()) ? IRB->CreatePtrToInt(Addr, AddrTy) : Addr;
  Value *SafeBeg  = ConstantInt::get(AddrTy, kSafeAlignedBeg);
  Value *SafeEnd  = ConstantInt::get(AddrTy, kSafeAlignedEnd);
  Value *Cmp1 = IRB->CreateICmpULE(SafeBeg, AddrLong);  // SafeBeg <= Addr
  Value *Cmp2 = IRB->CreateICmpULE(AddrLong, SafeEnd);  // Addr <= SafeEnd
  Value *Ret  = IRB->CreateLogicalAnd(Cmp1, Cmp2);
  return Ret;
}
static Value *createAddrIsInSafeUnalignedRegion(IRBuilder<> *IRB, Value *Addr) {
  Type *AddrTy = IRB->getInt64Ty();
  Value *AddrLong = (Addr->getType()->isPointerTy()) ? IRB->CreatePtrToInt(Addr, AddrTy) : Addr;
  Value *SafeBeg  = ConstantInt::get(AddrTy, kSafeUnalignedBeg);
  Value *SafeEnd  = ConstantInt::get(AddrTy, kSafeUnalignedEnd);
  Value *Cmp1 = IRB->CreateICmpULE(SafeBeg, AddrLong);  // SafeBeg <= Addr
  Value *Cmp2 = IRB->CreateICmpULE(AddrLong, SafeEnd);  // Addr <= SafeEnd
  Value *Ret  = IRB->CreateLogicalAnd(Cmp1, Cmp2);
  return Ret;
}

Instruction *DataFlowIntegritySanitizerPass::generateCrashCode(Instruction *InsertBefore, Value *Addr, ValueVector &DefIDs, bool IsUnsafe) {
  Builder->SetInsertPoint(InsertBefore);
  Value *AddrLong = Builder->CreatePtrToInt(Addr, Int64Ty);
  CallInst *Call = nullptr;
  if (IsUnsafe) {
    Call = Builder->CreateCall(InvalidSafeAccessReportFn, {AddrLong});
  } else {
    Value *Argc = ConstantInt::get(Int16Ty, DefIDs.size(), false);
    ValueVector Args{Addr, Argc};
    Args.append(DefIDs);
    Call = Builder->CreateCall(InvalidUseReportFn, Args);
  }
  return Call;
}

/* --- Unsafe Access --- */
void DataFlowIntegritySanitizerPass::instrumentUnsafeAccess(Instruction *OrigInst, Value *Addr) {
  Builder->SetInsertPoint(OrigInst);
  Value *Cmp = createAddrIsInSafeRegion(Builder.get(), Addr);
  Instruction *CrashTerm = SplitBlockAndInsertIfThen(Cmp, OrigInst, /* unreachable */ true);
  ValueVector Empty{};
  Instruction *Crash = generateCrashCode(CrashTerm, Addr, Empty, /* IsUnsafe */ true);
  Crash->setDebugLoc(OrigInst->getDebugLoc());

  NumInstrumentedUnsafeAccesses++;
}

/// Get shadow memroy address
static Value *AlignAddr(IRBuilder<> *IRB, Value *Addr) {
  Type *MaskTy = Addr->getType();
  uintptr_t MaskInt = ~((uintptr_t)kAlignedMemGranularity - 1);
  Value *MaskVal = ConstantInt::get(MaskTy, MaskInt); // clear lower 2-bits
  return IRB->CreateAnd(Addr, MaskVal);
}
static Value *AlignedMemToShadow(IRBuilder<> *IRB, Value *Addr) {
  Type *WidthTy = Addr->getType();
  Value *WidthVal = ConstantInt::get(WidthTy, kAlignedShiftWidth);
  return IRB->CreateLShr(Addr, WidthVal);
}
static Value *UnalignedMemToShadow(IRBuilder<> *IRB, Value *Addr) {
  Type *WidthTy = Addr->getType();
  Value *WidthVal = ConstantInt::get(WidthTy, kUnalignedShiftWidth);
  return IRB->CreateShl(Addr, WidthVal);
}
static Value *getNextShadowAddr(IRBuilder<> *IRB, Value *ShadowAddr, unsigned Num) {
  Type *OffsetTy = ShadowAddr->getType();
  Value *Offset = ConstantInt::get(OffsetTy, Num * 2);
  return IRB->CreateAdd(ShadowAddr, Offset);
}

/// Insert `if (AddrIsInSafeAlignedRegion(Addr)) {} else {}`
static void insertIfAddrIsInAlignedRegion(IRBuilder<> *IRB, Value *Addr, Instruction *InsertPoint, Instruction **Then, Instruction **Else) {
  Value *IsInAligned = createAddrIsInSafeAlignedRegion(IRB, Addr);
  SplitBlockAndInsertIfThenElse(IsInAligned, InsertPoint, Then, Else);
}
/// Insert `if (AddrIsInSafeUnalignedRegion(Addr)) {} else {}`
static void insertIfAddrIsInUnalignedRegion(IRBuilder<> *IRB, Value *Addr, Instruction *InsertPoint, Instruction **Then, Instruction **Else) {
  Value *IsInUnaligned = createAddrIsInSafeUnalignedRegion(IRB, Addr);
  SplitBlockAndInsertIfThenElse(IsInUnaligned, InsertPoint, Then, Else);
}
/// Insert `if (AddrIsInSafeAlignedRegion(Addr)) {} else if (AddrIsInSafeUnalignedRegion(Addr)) {} else {}`
static void insertIfAddrIsInAlignedRegionElseIfAddrIsInUnalignedRegion(IRBuilder<> *IRB, Value *Addr, Instruction *InsertPoint, Instruction **Then1, Instruction **Then2, Instruction **Else) {
  Instruction *Else1;
  insertIfAddrIsInAlignedRegion(IRB, Addr, InsertPoint, Then1, &Else1);
  IRB->SetInsertPoint(Else1);
  insertIfAddrIsInUnalignedRegion(IRB, Addr, Else1, Then2, Else);
}

/* --- Store functions --- */
static Value *setDefID(IRBuilder<> *IRB, Value *ShadowAddr, Value *DefID) {
  Type *PtrTy = PointerType::getInt16PtrTy(IRB->getContext());
  Value *ShadowPtr = IRB->CreateIntToPtr(ShadowAddr, PtrTy);
  return IRB->CreateStore(DefID, ShadowPtr);
}
static Value *setDefID(IRBuilder<> *IRB, Value *ShadowAddr, Value *DefID, unsigned Offset) {
  if (Offset != 0)
    ShadowAddr = getNextShadowAddr(IRB, ShadowAddr, Offset);
  return setDefID(IRB, ShadowAddr, DefID);
}

static void insertSet(IRBuilder<> *IRB, Instruction *InsertPoint, Value *ShadowAddr, Value *DefID, unsigned IDNum) {
  IRB->SetInsertPoint(InsertPoint);
  for (unsigned Offset = 0; Offset < IDNum; Offset++)
    setDefID(IRB, ShadowAddr, DefID, Offset);
}
static void insertAlignedSet(IRBuilder<> *IRB, Instruction *InsertPoint, Value *StoreAddr, Value *DefID, unsigned Size) {
  Value *ShadowAddr = AlignedMemToShadow(IRB, StoreAddr);
  unsigned IDNum = ceil((double)Size / (double)4);
  insertSet(IRB, InsertPoint, ShadowAddr, DefID, IDNum);
}
static void insertUnalignedSet(IRBuilder<> *IRB, Instruction *InsertPoint, Value *StoreAddr, Value *DefID, unsigned Size) {
  Value *ShadowAddr = UnalignedMemToShadow(IRB, StoreAddr);
  insertSet(IRB, InsertPoint, ShadowAddr, DefID, Size);
}

/// Aligned store
void DataFlowIntegritySanitizerPass::instrumentAlignedStore(Instruction *InsertPoint, Value *StoreAddr, Value *DefID, unsigned Size) {
  if (Size < 4)
    StoreAddr = AlignAddr(Builder.get(), StoreAddr);
  insertAlignedSet(Builder.get(), InsertPoint, StoreAddr, DefID, Size);
}
void DataFlowIntegritySanitizerPass::instrumentAlignedStoreN(Instruction *InsertPoint, Value *StoreAddr, Value *DefID, Value *SizeVal, unsigned Size) {
  // TODO
  // Value *LoopCnt = Builder->CreateAlloca(Int32Ty);
  // Value *InitVal = ConstantInt::get(Int32Ty, 0);
  // Builder->CreateStore(InitVal, LoopCnt);
  // Loop init
  Value *ZeroVal = ConstantInt::get(Int32Ty, 0);
  Value *LoopCnt = Builder->CreateAdd(ZeroVal, ZeroVal);
  Value *ShadowAddr = Builder->CreateAdd(StoreAddr, ConstantInt::get(StoreAddr->getType(), 0));
  // Value *FalseVal = Builder->getFalse();
  // Value *IsAttack = Builder->CreateLogicalAnd(FalseVal, FalseVal);
  // Loop cond
  BasicBlock *CondBlock = SplitBlock(InsertPoint->getParent(), InsertPoint);
  Builder->SetInsertPoint(CondBlock);
  // Value *Idx = Builder->CreateLoad(Int32Ty, LoopCnt);
  Value *Cond = Builder->CreateICmpNE(LoopCnt, SizeVal);
  // Body
  Instruction *Then = SplitBlockAndInsertIfThen(Cond, InsertPoint, /* unreachable */ false);
  Builder->SetInsertPoint(Then);
}

/// Unaligned store
void DataFlowIntegritySanitizerPass::instrumentUnalignedStore(Instruction *InsertPoint, Value *StoreAddr, Value *DefID, unsigned Size) {
  insertUnalignedSet(Builder.get(), InsertPoint, StoreAddr, DefID, Size);
}

/// Aligned or unaligned store
void DataFlowIntegritySanitizerPass::instrumentAlignedOrUnalignedStore(Instruction *InsertPoint, Value *StoreAddr, Value *DefID, unsigned Size) {
  Instruction *Then, *Else;
  insertIfAddrIsInAlignedRegion(Builder.get(), StoreAddr, InsertPoint, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentAlignedStore(Then, StoreAddr, DefID, Size);
  Builder->SetInsertPoint(Else);
  instrumentUnalignedStore(Else, StoreAddr, DefID, Size);
}
void DataFlowIntegritySanitizerPass::instrumentAlignedOrUnalignedStoreN(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  // TODO
}

/// Conditional aligned
void DataFlowIntegritySanitizerPass::instrumentCondAlignedStore(Instruction *InsertPoint, Value *StoreAddr, Value *DefID, unsigned Size) {
  Instruction *Then, *Else;
  insertIfAddrIsInAlignedRegion(Builder.get(), StoreAddr, InsertPoint, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentAlignedStore(Then, StoreAddr, DefID, Size);
}

/// Conditional unaligned
void DataFlowIntegritySanitizerPass::instrumentCondUnalignedStore(Instruction *InsertPoint, Value *StoreAddr, Value *DefID, unsigned Size) {
  Instruction *Then, *Else;
  insertIfAddrIsInUnalignedRegion(Builder.get(), StoreAddr, InsertPoint, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentUnalignedStore(Then, StoreAddr, DefID, Size);
}

/// Conditional aligned or unaligned
void DataFlowIntegritySanitizerPass::instrumentCondAlignedOrUnalignedStore(Instruction *InsertPoint, Value *StoreAddr, Value *DefID, unsigned Size) {
  Instruction *Then1, *Then2, *Else;
  insertIfAddrIsInAlignedRegionElseIfAddrIsInUnalignedRegion(Builder.get(), StoreAddr, InsertPoint, &Then1, &Then2, &Else);
  Builder->SetInsertPoint(Then1);
  instrumentAlignedStore(Then1, StoreAddr, DefID, Size);
  Builder->SetInsertPoint(Then2);
  instrumentAlignedStore(Then2, StoreAddr, DefID, Size);
}


/* --- Check functions --- */
/// Get DefID
static Value *getDefID(IRBuilder<> *IRB, Value *ShadowAddr) {
  Type *DefIDTy = IRB->getInt16Ty();
  Type *PtrTy = PointerType::getInt16PtrTy(IRB->getContext());
  Value *ShadowPtr = IRB->CreateIntToPtr(ShadowAddr, PtrTy);
  return IRB->CreateLoad(DefIDTy, ShadowPtr);
}
static Value *getDefID(IRBuilder<> *IRB, Value *ShadowAddr, unsigned Offset) {
  if (Offset != 0)
    ShadowAddr = getNextShadowAddr(IRB, ShadowAddr, Offset);
  return getDefID(IRB, ShadowAddr);
}
static Value *getNextDefID(IRBuilder<> *IRB, Value *ShadowAddr, unsigned Num) {
  Value *NextShadowAddr = getNextShadowAddr(IRB, ShadowAddr, Num);
  return getDefID(IRB, NextShadowAddr);
}

/// Atomic exchange of DefID
static Value *exchangeDefID(IRBuilder<> *IRB, Value *ShadowAddr, Value *DefID) {
  // Type *DefIDTy = IRB->getInt16Ty();
  Type *PtrTy = PointerType::getInt16PtrTy(IRB->getContext());
  Value *ShadowPtr = IRB->CreateIntToPtr(ShadowAddr, PtrTy);
  return IRB->CreateAtomicRMW(AtomicRMWInst::BinOp::Xchg, ShadowPtr, DefID, MaybeAlign(2), AtomicOrdering(AtomicOrdering::Monotonic) /* same as C++'s relaxed */);
}
static Value *exchangeDefID(IRBuilder<> *IRB, Value *ShadowAddr, Value *DefID, unsigned Offset) {
  if (Offset != 0)
    ShadowAddr = getNextShadowAddr(IRB, ShadowAddr, Offset);
  return exchangeDefID(IRB, ShadowAddr, DefID);
}
static Value *exchangeNextDefID(IRBuilder<> *IRB, Value *ShadowAddr, Value *DefID, unsigned Num) {
  Value *NextShadowAddr = getNextShadowAddr(IRB, ShadowAddr, Num);
  return exchangeDefID(IRB, NextShadowAddr, DefID);
}

/// Compare two DefIDs.
static Value *compareDefIDs(IRBuilder<> *IRB, Value *Lhs, Value *Rhs) {
  assert(Lhs->getType() == Rhs->getType());
  return IRB->CreateICmpEQ(Lhs, Rhs);
}
static Value *compareDefIDsAndSetResult(IRBuilder<> *IRB, Value *Res, Value *Lhs, Value *Rhs) {
  Value *CmpRes = compareDefIDs(IRB, Lhs, Rhs);
  assert(Res->getType() == CmpRes->getType());
  return IRB->CreateLogicalOr(Res, CmpRes);
}

/// Check DefIDs and Return the result
static Value *checkDefIDs(IRBuilder<> *IRB, Value *CurrDefID, ValueVector &DefIDs) {
  if (DefIDs.size() == 1)
    return IRB->CreateICmpNE(CurrDefID, DefIDs[0]);

  Value *IsOK = ConstantInt::getFalse(IRB->getContext());
  for (auto *DefID : DefIDs) {
    IsOK = compareDefIDsAndSetResult(IRB, IsOK, CurrDefID, DefID);
  }
  Value *IsAttack = IRB->CreateNot(IsOK);
  return IsAttack;
}

static void getCurrDefIDs(IRBuilder<> *IRB, Value *ShadowAddr, unsigned IDNum, ValueVector &CurrDefIDs) {
  for (unsigned Offset = 0; Offset < IDNum; Offset++)
    CurrDefIDs.push_back(getDefID(IRB, ShadowAddr, Offset));
}

static void exchangeCurrDefIDs(IRBuilder<> *IRB, Value *ShadowAddr, Value *DefID, unsigned IDNum, ValueVector &CurrDefIDs) {
  for (unsigned Offset = 0; Offset < IDNum; Offset++)
    CurrDefIDs.push_back(exchangeDefID(IRB, ShadowAddr, DefID, Offset));
}

static Value *insertCheck(IRBuilder<> *IRB, Instruction *InsertPoint, Value *ShadowAddr, ValueVector &DefIDs, ValueVector &CurrDefIDs) {
  ValueVector AttackResults;
  for (auto *CurrDefID : CurrDefIDs)
    AttackResults.push_back(checkDefIDs(IRB, CurrDefID, DefIDs));
  SmallVector<ValueVector, 8> ResultMap(CurrDefIDs.size());
  ResultMap[0] = AttackResults;
  unsigned Idx = 0;
  while (ResultMap[Idx].size() != 1) {
    unsigned NextIdx = Idx + 1;
    for (unsigned Idx2 = 0; Idx2 < ResultMap[Idx].size(); Idx2 += 2) {
      Value *IsAttack0 = ResultMap[Idx][Idx2];
      if (ResultMap[Idx].size() <= Idx2 + 1) {
        ResultMap[NextIdx].push_back(IsAttack0);
      } else {
        Value *IsAttack1 = ResultMap[Idx][Idx2 + 1];
        Value *IsAttack  = IRB->CreateLogicalOr(IsAttack0, IsAttack1);
        ResultMap[NextIdx].push_back(IsAttack);
      }
    }
    Idx = NextIdx;
  }
  assert(ResultMap[Idx].size() == 1);
  return ResultMap[Idx][0];
}

static Value *insertAlignedCheck(IRBuilder<> *IRB, Instruction *InsertPoint, Value *LoadAddr, ValueVector &DefIDs, unsigned Size) {
  IRB->SetInsertPoint(InsertPoint);
  Value *ShadowAddr = AlignedMemToShadow(IRB, LoadAddr);
  unsigned IDNum = ceil((double)Size / (double)4);

  ValueVector CurrDefIDs;
  getCurrDefIDs(IRB, ShadowAddr, IDNum, CurrDefIDs);
  return insertCheck(IRB, InsertPoint, ShadowAddr, DefIDs, CurrDefIDs);
}

static Value *insertUnalignedCheck(IRBuilder<> *IRB, Instruction *InsertPoint, Value *LoadAddr, ValueVector &DefIDs, unsigned Size) {
  IRB->SetInsertPoint(InsertPoint);
  Value *ShadowAddr = UnalignedMemToShadow(IRB, LoadAddr);

  ValueVector CurrDefIDs;
  getCurrDefIDs(IRB, ShadowAddr, Size, CurrDefIDs);
  return insertCheck(IRB, InsertPoint, ShadowAddr, DefIDs, CurrDefIDs);
}

/// Insert if-then to report error
void DataFlowIntegritySanitizerPass::insertIfThenAndErrorReport(Value *Cond, Instruction *InsertPoint, Value *LoadAddr, ValueVector &DefIDs) {
  Instruction *CrashTerm = SplitBlockAndInsertIfThen(Cond, InsertPoint, /* unreachable */ true);
  Instruction *Crash = generateCrashCode(CrashTerm, LoadAddr, DefIDs);
  Crash->setDebugLoc(InsertPoint->getDebugLoc());
}

/// Aligned check
void DataFlowIntegritySanitizerPass::instrumentAlignedLoad(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs, unsigned Size) {
  if (Size < 4)
    LoadAddr = AlignAddr(Builder.get(), LoadAddr);
  Value *IsAttack = insertAlignedCheck(Builder.get(), Load, LoadAddr, DefIDs, Size);
  insertIfThenAndErrorReport(IsAttack, Load, LoadAddr, DefIDs);
}
void DataFlowIntegritySanitizerPass::instrumentAlignedLoadN(Instruction *Load, Value *LoadAddr, unsigned Size, ValueVector &DefIDs) {
  // TODO
}

/// Unaligned check
void DataFlowIntegritySanitizerPass::instrumentUnalignedLoad(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs, unsigned Size) {
  Value *IsAttack = insertUnalignedCheck(Builder.get(), Load, LoadAddr, DefIDs, Size);
  insertIfThenAndErrorReport(IsAttack, Load, LoadAddr, DefIDs);
}
void DataFlowIntegritySanitizerPass::instrumentUnalignedLoadN(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  // TODO
}

/// Aligned or unaligned
void DataFlowIntegritySanitizerPass::instrumentAlignedOrUnalignedLoad(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs, unsigned Size) {
  Instruction *Then, *Else;
  insertIfAddrIsInAlignedRegion(Builder.get(), LoadAddr, Load, &Then, &Else);
  instrumentAlignedLoad(Then, LoadAddr, DefIDs, Size);
  instrumentUnalignedLoad(Else, LoadAddr, DefIDs, Size);
}

/// Conditional aligned
void DataFlowIntegritySanitizerPass::instrumentCondAlignedLoad(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs, unsigned Size) {
  Instruction *Then, *Else;
  insertIfAddrIsInAlignedRegion(Builder.get(), LoadAddr, Load, &Then, &Else);
  instrumentAlignedLoad(Then, LoadAddr, DefIDs, Size);
}

/// Conditional unaligned
void DataFlowIntegritySanitizerPass::instrumentCondUnalignedLoad(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs, unsigned Size) {
  Instruction *Then, *Else;
  insertIfAddrIsInUnalignedRegion(Builder.get(), LoadAddr, Load, &Then, &Else);
  instrumentUnalignedLoad(Then, LoadAddr, DefIDs, Size);
}

/// Conditional aligned or unaligned
void DataFlowIntegritySanitizerPass::instrumentCondAlignedOrUnalignedLoad(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs, unsigned Size) {
  Instruction *Then1, *Then2, *Else;
  insertIfAddrIsInAlignedRegionElseIfAddrIsInUnalignedRegion(Builder.get(), LoadAddr, Load, &Then1, &Then2, &Else);
  instrumentAlignedLoad(Then1, LoadAddr, DefIDs, Size);
  instrumentUnalignedLoad(Then2, LoadAddr, DefIDs, Size);
}

static Value *insertAlignedCheckSet(IRBuilder<> *IRB, Instruction *InsertPoint, Value *Addr, Value *DefID, ValueVector &DefIDs, unsigned Size) {
  Value *ShadowAddr = AlignedMemToShadow(IRB, Addr);
  unsigned IDNum = ceil((double)Size / (double)4);
  IRB->SetInsertPoint(InsertPoint);

  ValueVector CurrDefIDs;
  exchangeCurrDefIDs(IRB, ShadowAddr, DefID, IDNum, CurrDefIDs);
  return insertCheck(IRB, InsertPoint, ShadowAddr, DefIDs, CurrDefIDs);
}

static Value *insertUnalignedCheckSet(IRBuilder<> *IRB, Instruction *InsertPoint, Value *Addr, Value *DefID, ValueVector &DefIDs, unsigned Size) {
  Value *ShadowAddr = UnalignedMemToShadow(IRB, Addr);
  IRB->SetInsertPoint(InsertPoint);

  ValueVector CurrDefIDs;
  exchangeCurrDefIDs(IRB, ShadowAddr, DefID, Size, CurrDefIDs);
  return insertCheck(IRB, InsertPoint, ShadowAddr, DefIDs, CurrDefIDs);
}

void DataFlowIntegritySanitizerPass::instrumentAlignedRaceStore(Instruction *InsertPoint, Value *Addr, Value *DefID, ValueVector &DefIDs, unsigned Size) {
  if (Size < 4)
    Addr = AlignAddr(Builder.get(), Addr);
  Value *IsAttack = insertAlignedCheckSet(Builder.get(), InsertPoint, Addr, DefID, DefIDs, Size);
  insertIfThenAndErrorReport(IsAttack, InsertPoint, Addr, DefIDs);
}

void DataFlowIntegritySanitizerPass::instrumentUnalignedRaceStore(Instruction *InsertPoint, Value *Addr, Value *DefID, ValueVector &DefIDs, unsigned Size) {
  Value *IsAttack = insertUnalignedCheckSet(Builder.get(), InsertPoint, Addr, DefID, DefIDs, Size);
  insertIfThenAndErrorReport(IsAttack, InsertPoint, Addr, DefIDs);
}

void DataFlowIntegritySanitizerPass::instrumentAlignedOrUnalignedRaceStore(Instruction *InsertPoint, Value *Addr, Value *DefID, ValueVector &DefIDs, unsigned Size) {
  Instruction *Then, *Else;
  insertIfAddrIsInAlignedRegion(Builder.get(), Addr, InsertPoint, &Then, &Else);
  instrumentAlignedRaceStore(Then, Addr, DefID, DefIDs, Size);
  instrumentUnalignedRaceStore(Else, Addr, DefID, DefIDs, Size);
}

void DataFlowIntegritySanitizerPass::instrumentCondAlignedRaceStore(Instruction *InsertPoint, Value *Addr, Value *DefID, ValueVector &DefIDs, unsigned Size) {
  Instruction *Then, *Else;
  insertIfAddrIsInAlignedRegion(Builder.get(), Addr, InsertPoint, &Then, &Else);
  instrumentAlignedRaceStore(Then, Addr, DefID, DefIDs, Size);
}

void DataFlowIntegritySanitizerPass::instrumentCondUnalignedRaceStore(Instruction *InsertPoint, Value *Addr, Value *DefID, ValueVector &DefIDs, unsigned Size) {
  Instruction *Then, *Else;
  insertIfAddrIsInAlignedRegion(Builder.get(), Addr, InsertPoint, &Then, &Else);
  instrumentUnalignedRaceStore(Then, Addr, DefID, DefIDs, Size);
}

void DataFlowIntegritySanitizerPass::instrumentCondAlignedOrUnalignedRaceStore(Instruction *InsertPoint, Value *Addr, Value *DefID, ValueVector &DefIDs, unsigned Size) {
  Instruction *Then1, *Then2, *Else;
  insertIfAddrIsInAlignedRegionElseIfAddrIsInUnalignedRegion(Builder.get(), Addr, InsertPoint, &Then1, &Then2, &Else);
  instrumentAlignedRaceStore(Then1, Addr, DefID, DefIDs, Size);
  instrumentUnalignedRaceStore(Then2, Addr, DefID, DefIDs, Size);
}

/// Aligned check

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

  auto &Result = MAM.getResult<SVFDefUseAnalysisPass>(M);
  ProtInfo = Result.getProtectInfo();

  // Instrument unsafe access
  for (auto &Iter : ProtInfo->getUnsafeToInfo()) {
    Instruction *UnsafeInst = Iter.first;
    for (auto &Ope : Iter.second.Operands) {
      //   ** InsertPoint **
      //   UnsafeInst: store %0 to <Operand>
      Value *OpeVal = Ope.Operand;
      instrumentUnsafeAccess(UnsafeInst, OpeVal);
    }
  }

  if (ProtInfo->emptyTarget())
    return PreservedAnalyses::none();
  
  // Instrument store functions.
  for (auto *AlignedOnlyDef : ProtInfo->getAlignedOnlyDefs()) {
    insertDfiStoreFn(AlignedOnlyDef, UseDefKind::Aligned);
  }
  for (auto *UnalignedOnlyDef : ProtInfo->getUnalignedOnlyDefs()) {
    insertDfiStoreFn(UnalignedOnlyDef, UseDefKind::Unaligned);
  }
  for (auto *BothOnlyDef : ProtInfo->getBothOnlyDefs()) {
    insertDfiStoreFn(BothOnlyDef, UseDefKind::AlignedOrUnaligned);
  }
  for (auto *AlignedOrNoTargetDef : ProtInfo->getAlignedOrNoTargetDefs()) {
    insertDfiStoreFn(AlignedOrNoTargetDef, UseDefKind::CondAligned);
  }
  for (auto *UnalignedOrNoTargetDef : ProtInfo->getUnalignedOrNoTargetDefs()) {
    insertDfiStoreFn(UnalignedOrNoTargetDef, UseDefKind::CondUnaligned);
  }
  for (auto *BothOrNoTargetDef : ProtInfo->getBothOrNoTargetDefs()) {
    insertDfiStoreFn(BothOrNoTargetDef, UseDefKind::CondAlignedOrUnaligned);
  }

  // Instrument check functions.
  for (auto *AlignedOnlyUse : ProtInfo->getAlignedOnlyUses()) {
    insertDfiLoadFn(AlignedOnlyUse, UseDefKind::Aligned);
  }
  for (auto *UnalignedOnlyUse : ProtInfo->getUnalignedOnlyUses()) {
    insertDfiLoadFn(UnalignedOnlyUse, UseDefKind::Unaligned);
  }
  for (auto *BothOnlyUse : ProtInfo->getBothOnlyUses()) {
    insertDfiLoadFn(BothOnlyUse, UseDefKind::AlignedOrUnaligned);
  }
  for (auto *AlignedOrNoTargetUse : ProtInfo->getAlignedOrNoTargetUses()) {
    insertDfiLoadFn(AlignedOrNoTargetUse, UseDefKind::CondAligned);
  }
  for (auto *UnalignedOrNoTargetUse : ProtInfo->getUnalignedOrNoTargetUses()) {
    insertDfiLoadFn(UnalignedOrNoTargetUse, UseDefKind::CondUnaligned);
  }
  for (auto *BothOrNoTargetUse : ProtInfo->getBothOrNoTargetUses()) {
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

  // Store Fn Type
  SmallVector<Type *, 8> StoreArgTypes{PtrTy, Int16Ty};
  FunctionType *StoreFnTy = FunctionType::get(VoidTy, StoreArgTypes, false);
  SmallVector<Type *, 8> StoreNArgTypes{PtrTy, Int64Ty, Int16Ty};
  FunctionType *StoreNFnTy = FunctionType::get(VoidTy, StoreNArgTypes, false);
  // Load Fn Type
  // SmallVector<Type *, 8> LoadArgTypes{PtrTy, Int32Ty};
  SmallVector<Type *, 8> LoadArgTypes{PtrTy, Int16Ty};
  FunctionType *LoadFnTy = FunctionType::get(VoidTy, LoadArgTypes, true);
  // SmallVector<Type *, 8> LoadNArgTypes{PtrTy, Int64Ty, Int32Ty};
  SmallVector<Type *, 8> LoadNArgTypes{PtrTy, Int64Ty, Int16Ty};
  FunctionType *LoadNFnTy = FunctionType::get(VoidTy, LoadNArgTypes, true);

  // Check and Set Fn Type
  SmallVector<Type *, 8> LoadStoreArgTypes{PtrTy, Int16Ty, Int16Ty};
  FunctionType *LoadStoreFnTy = FunctionType::get(VoidTy, LoadStoreArgTypes, true);
  SmallVector<Type *, 8> LoadStoreNArgTypes{PtrTy, Int16Ty, Int64Ty, Int16Ty};
  FunctionType *LoadStoreNFnTy = FunctionType::get(VoidTy, LoadStoreNArgTypes, true);

  // Other Fn Type
  FunctionType *CheckUnsafeAcessFnTy = FunctionType::get(VoidTy, {Int64Ty}, false);
  FunctionType *InvalidSafeAccessReportFnTy = FunctionType::get(VoidTy, {Int64Ty}, false);
  FunctionType *InvalidUseReportFnTy = FunctionType::get(VoidTy, {PtrTy, Int16Ty}, true);

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

  // Load and Store functions
  // aligned
  AlignedLoadStoreNFn = M->getOrInsertFunction(DfisanAlignedLoadStoreNFnName, LoadStoreNFnTy);
  AlignedLoadStore1Fn = M->getOrInsertFunction(DfisanAlignedLoadStore1FnName, LoadStoreFnTy);
  AlignedLoadStore2Fn = M->getOrInsertFunction(DfisanAlignedLoadStore2FnName, LoadStoreFnTy);
  AlignedLoadStore4Fn = M->getOrInsertFunction(DfisanAlignedLoadStore4FnName, LoadStoreFnTy);
  AlignedLoadStore8Fn = M->getOrInsertFunction(DfisanAlignedLoadStore8FnName, LoadStoreFnTy);
  AlignedLoadStore16Fn = M->getOrInsertFunction(DfisanAlignedLoadStore16FnName, LoadStoreFnTy);
  // unaligned
  UnalignedLoadStoreNFn = M->getOrInsertFunction(DfisanUnalignedLoadStoreNFnName, LoadStoreNFnTy);
  UnalignedLoadStore1Fn = M->getOrInsertFunction(DfisanUnalignedLoadStore1FnName, LoadStoreFnTy);
  UnalignedLoadStore2Fn = M->getOrInsertFunction(DfisanUnalignedLoadStore2FnName, LoadStoreFnTy);
  UnalignedLoadStore4Fn = M->getOrInsertFunction(DfisanUnalignedLoadStore4FnName, LoadStoreFnTy);
  UnalignedLoadStore8Fn = M->getOrInsertFunction(DfisanUnalignedLoadStore8FnName, LoadStoreFnTy);
  UnalignedLoadStore16Fn = M->getOrInsertFunction(DfisanUnalignedLoadStore16FnName, LoadStoreFnTy);
  // aligned or unaligned
  AlignedOrUnalignedLoadStoreNFn = M->getOrInsertFunction(DfisanAlignedOrUnalignedLoadStoreNFnName, LoadStoreNFnTy);
  AlignedOrUnalignedLoadStore1Fn = M->getOrInsertFunction(DfisanAlignedOrUnalignedLoadStore1FnName, LoadStoreFnTy);
  AlignedOrUnalignedLoadStore2Fn = M->getOrInsertFunction(DfisanAlignedOrUnalignedLoadStore2FnName, LoadStoreFnTy);
  AlignedOrUnalignedLoadStore4Fn = M->getOrInsertFunction(DfisanAlignedOrUnalignedLoadStore4FnName, LoadStoreFnTy);
  AlignedOrUnalignedLoadStore8Fn = M->getOrInsertFunction(DfisanAlignedOrUnalignedLoadStore8FnName, LoadStoreFnTy);
  AlignedOrUnalignedLoadStore16Fn = M->getOrInsertFunction(DfisanAlignedOrUnalignedLoadStore16FnName, LoadStoreFnTy);
  // conditional aligned
  CondAlignedLoadStoreNFn = M->getOrInsertFunction(DfisanCondAlignedLoadStoreNFnName, LoadStoreNFnTy);
  CondAlignedLoadStore1Fn = M->getOrInsertFunction(DfisanCondAlignedLoadStore1FnName, LoadStoreFnTy);
  CondAlignedLoadStore2Fn = M->getOrInsertFunction(DfisanCondAlignedLoadStore2FnName, LoadStoreFnTy);
  CondAlignedLoadStore4Fn = M->getOrInsertFunction(DfisanCondAlignedLoadStore4FnName, LoadStoreFnTy);
  CondAlignedLoadStore8Fn = M->getOrInsertFunction(DfisanCondAlignedLoadStore8FnName, LoadStoreFnTy);
  CondAlignedLoadStore16Fn = M->getOrInsertFunction(DfisanCondAlignedLoadStore16FnName, LoadStoreFnTy);
  // conditional unaligned
  CondUnalignedLoadStoreNFn = M->getOrInsertFunction(DfisanCondUnalignedLoadStoreNFnName, LoadStoreNFnTy);
  CondUnalignedLoadStore1Fn = M->getOrInsertFunction(DfisanCondUnalignedLoadStore1FnName, LoadStoreFnTy);
  CondUnalignedLoadStore2Fn = M->getOrInsertFunction(DfisanCondUnalignedLoadStore2FnName, LoadStoreFnTy);
  CondUnalignedLoadStore4Fn = M->getOrInsertFunction(DfisanCondUnalignedLoadStore4FnName, LoadStoreFnTy);
  CondUnalignedLoadStore8Fn = M->getOrInsertFunction(DfisanCondUnalignedLoadStore8FnName, LoadStoreFnTy);
  CondUnalignedLoadStore16Fn = M->getOrInsertFunction(DfisanCondUnalignedLoadStore16FnName, LoadStoreFnTy);
  // conditional aligned or unaligned
  CondAlignedOrUnalignedLoadStoreNFn = M->getOrInsertFunction(DfisanCondAlignedOrUnalignedLoadStoreNFnName, LoadStoreNFnTy);
  CondAlignedOrUnalignedLoadStore1Fn = M->getOrInsertFunction(DfisanCondAlignedOrUnalignedLoadStore1FnName, LoadStoreFnTy);
  CondAlignedOrUnalignedLoadStore2Fn = M->getOrInsertFunction(DfisanCondAlignedOrUnalignedLoadStore2FnName, LoadStoreFnTy);
  CondAlignedOrUnalignedLoadStore4Fn = M->getOrInsertFunction(DfisanCondAlignedOrUnalignedLoadStore4FnName, LoadStoreFnTy);
  CondAlignedOrUnalignedLoadStore8Fn = M->getOrInsertFunction(DfisanCondAlignedOrUnalignedLoadStore8FnName, LoadStoreFnTy);
  CondAlignedOrUnalignedLoadStore16Fn = M->getOrInsertFunction(DfisanCondAlignedOrUnalignedLoadStore16FnName, LoadStoreFnTy);

  // Unsafe access check
  CheckUnsafeAccessFn = M->getOrInsertFunction(DfisanCheckUnsafeAccessFnName, CheckUnsafeAcessFnTy);

  // Error report
  InvalidSafeAccessReportFn = M->getOrInsertFunction(DfisanInvalidSafeAccessReportFnName, InvalidSafeAccessReportFnTy);
  InvalidUseReportFn        = M->getOrInsertFunction(DfisanInvalidUseReportFnName, InvalidUseReportFnTy);
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

  const auto &Info = ProtInfo->getDefInfo(Def);
  if (Info.IsWriteWriteRace)
    return instrumentMayWriteWriteRaceStore(Def, Kind);

  if (Instruction *DefInst = dyn_cast<Instruction>(Def)) {
    for (const auto &Ope : Info.Operands) {
      if (Ope.hasSizeVal()) {
        LLVM_DEBUG(dbgs() << " - ID(" << Info.ID << "), Operand: " << *Ope.Operand << ", SizeVal: " << *Ope.SizeVal << "\n");
        // calloc handling
        //   DefInst: %0 = calloc()
        //   Operand: %1 = bitcast %0 to <type>
        //   ** InsertPoint **
        if (DefInst->getNextNonDebugInstruction() == Ope.Operand)
          createDfiStoreFn(Info.ID, Ope.Operand, Ope.SizeVal, Kind, DefInst->getNextNonDebugInstruction()->getNextNode());
        // other
        //   DefInst: store %0 to <Operand>
        //   ** InsertPoint **
        else
          createDfiStoreFn(Info.ID, Ope.Operand, Ope.SizeVal, Kind, DefInst->getNextNode());
      } else {
        LLVM_DEBUG(dbgs() << " - ID(" << Info.ID << "), Operand: " << *Ope.Operand << ", Size: " << Ope.Size << "\n");
        createDfiStoreFn(Info.ID, Ope.Operand, Ope.Size, Kind, DefInst->getNextNode());
      }
    }
  } else if (GlobalVariable *GlobVar = dyn_cast<GlobalVariable>(Def)) {
    assert(Info.Operands.size() == 1 && "GlobalVariable init has a single operand");
    auto &Ope = *Info.Operands.begin();
    createDfiStoreFn(Info.ID, GlobVar, Ope.Size, Kind, Ctor->getEntryBlock().getTerminator());
    insertGlobalInit(GlobVar);
  } else {
    llvm::errs() << "No support Def: " << *Def << "\n";
    // llvm_unreachable("No support Def");
  }
}

/// Insert CHECK and SET functions before each may-write-write-race store statement.
void DataFlowIntegritySanitizerPass::instrumentMayWriteWriteRaceStore(Value *Def, UseDefKind Kind) {
  const auto &Info = ProtInfo->getDefInfo(Def);
  assert(Info.IsWriteWriteRace);
  if (Instruction *DefInst = dyn_cast<Instruction>(Def)) {
    for (const auto &Ope : Info.Operands) {
      ValueVector DefIDs;
      for (const auto DefID : Info.DefIDs) {
        Value *IDVal = ConstantInt::get(Int16Ty, DefID, false);
        DefIDs.push_back(IDVal);
      }
      if (Ope.hasSizeVal()) {
        LLVM_DEBUG(dbgs() << " - ID(" << Info.ID << "), Operand: " << *Ope.Operand << ", SizeVal: " << *Ope.SizeVal << "\n");
        // createDfiLoadFn(Ope.Operand, Ope.SizeVal, DefIDs, Kind, DefInst);
        // createDfiStoreFn(Info.ID, Ope.Operand, Ope.SizeVal, Kind, DefInst);
        createDfiLoadStoreFn(Info.ID, Ope.Operand, Ope.SizeVal, DefIDs, Kind, DefInst);
      } else {
        LLVM_DEBUG(dbgs() << " - ID(" << Info.ID << "), Operand: " << *Ope.Operand << ", Size: " << Ope.Size << "\n");
        // createDfiLoadFn(Ope.Operand, Ope.Size, DefIDs, Kind, DefInst);
        // createDfiStoreFn(Info.ID, Ope.Operand, Ope.Size, Kind, DefInst);
        createDfiLoadStoreFn(Info.ID, Ope.Operand, Ope.Size, DefIDs, Kind, DefInst);
      }
    }
  } else {
    llvm_unreachable("No race Def");
  }
}

/// Insert a CHECK function before each load statement.
void DataFlowIntegritySanitizerPass::insertDfiLoadFn(Value *Use, UseDefKind Kind) {
  LLVM_DEBUG(llvm::dbgs() << "InsertDfiLoadFn: " << *Use << "\n");

  const auto &Info = ProtInfo->getUseInfo(Use);
  ValueVector DefIDs;
  for (const auto DefID : Info.DefIDs) {
    Value *IDVal = ConstantInt::get(Int16Ty, DefID, false);
    DefIDs.push_back(IDVal);
  }

  if (Instruction *UseInst = dyn_cast<Instruction>(Use)) {
    for (auto &Ope : Info.Operands) {
      if (Ope.hasSizeVal())
        createDfiLoadFn(Ope.Operand, Ope.SizeVal, DefIDs, Kind, UseInst);
      else
        createDfiLoadFn(Ope.Operand, Ope.Size, DefIDs, Kind, UseInst);
    }
  } else {
    llvm::errs() << "No support Use: " << *Use << "\n";
    llvm_unreachable("No support Use");
  }
}

static bool inlineCheckCode(unsigned Size) {
  return Size != 0 && Size <= 32;
}

/// Create a function call to DfiStoreFn.
void DataFlowIntegritySanitizerPass::createDfiStoreFn(DefID DefID, Value *StoreTarget, unsigned Size, UseDefKind Kind, Instruction *InsertPoint) {
  Value *SizeVal = ConstantInt::get(Int64Ty, Size, false);
  createDfiStoreFn(DefID, StoreTarget, SizeVal, Kind, InsertPoint);
}

void DataFlowIntegritySanitizerPass::createDfiStoreFn(DefID DefID, Value *StoreTarget, Value *SizeVal, UseDefKind Kind, Instruction *InsertPoint) {
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
  LLVM_DEBUG(dbgs() << "StoreAddr: " << *StoreAddr << "\n");
  LLVM_DEBUG(dbgs() << "Size(" << Size << "), " "SizeArg: " << *SizeArg << "\n");

  if (ClCheckWithCall) {
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
    }
  } else if (inlineCheckCode(Size)) {
    switch(Kind) {
    case UseDefKind::Aligned:
      instrumentAlignedStore(InsertPoint, StoreAddr, DefIDVal, Size);
      NumInstrumentedAlignedStores++;
      break;
    case UseDefKind::Unaligned:
      instrumentUnalignedStore(InsertPoint, StoreAddr, DefIDVal, Size);
      NumInstrumentedUnalignedStores++;
      break;
    case UseDefKind::AlignedOrUnaligned:
      instrumentAlignedOrUnalignedStore(InsertPoint, StoreAddr, DefIDVal, Size);
      NumInstrumentedAlignedOrUnalignedStores++;
      break;
    case UseDefKind::CondAligned:
      instrumentCondAlignedStore(InsertPoint, StoreAddr, DefIDVal, Size);
      NumInstrumentedCondAlignedStores++;
      break;
    case UseDefKind::CondUnaligned:
      instrumentCondUnalignedStore(InsertPoint, StoreAddr, DefIDVal, Size);
      NumInstrumentedCondUnalignedStores++;
      break;
    case UseDefKind::CondAlignedOrUnaligned:
      instrumentCondAlignedOrUnalignedStore(InsertPoint, StoreAddr, DefIDVal, Size);
      NumInstrumentedCondAlignedOrUnalignedStores++;
      break;
    }
  } else {
    switch(Kind) {
    case UseDefKind::Aligned:
      Builder->CreateCall(AlignedStoreNFn, NArgs);
      NumInstrumentedAlignedStores++;
      break;
    case UseDefKind::Unaligned:
      Builder->CreateCall(UnalignedStoreNFn, NArgs);
      NumInstrumentedUnalignedStores++;
      break;
    case UseDefKind::AlignedOrUnaligned:
      Builder->CreateCall(AlignedOrUnalignedStoreNFn, NArgs);
      NumInstrumentedAlignedOrUnalignedStores++;
      break;
    case UseDefKind::CondAligned:
      Builder->CreateCall(CondAlignedStoreNFn, NArgs);
      NumInstrumentedCondAlignedStores++;
      break;
    case UseDefKind::CondUnaligned:
      Builder->CreateCall(CondUnalignedStoreNFn, NArgs);
      NumInstrumentedCondUnalignedStores++;
      break;
    case UseDefKind::CondAlignedOrUnaligned:
      Builder->CreateCall(CondAlignedOrUnalignedStoreNFn, NArgs);
      NumInstrumentedCondAlignedOrUnalignedStores++;
      break;
    }
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
  // Value *Argc = ConstantInt::get(Int32Ty, DefIDs.size(), false);
  Value *Argc = ConstantInt::get(Int16Ty, DefIDs.size(), false);

  ValueVector Args{LoadAddr, Argc};
  Args.append(DefIDs);
  ValueVector NArgs{LoadAddr, SizeArg, Argc};
  NArgs.append(DefIDs);

  if (ClCheckWithCall) {
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
    }
  } else if (inlineCheckCode(Size)) {
    switch(Kind) {
    case UseDefKind::Aligned:
      instrumentAlignedLoad(InsertPoint, LoadAddr, DefIDs, Size);
      NumInstrumentedAlignedLoads++;
      break;
    case UseDefKind::Unaligned:
      instrumentUnalignedLoad(InsertPoint, LoadAddr, DefIDs, Size);
      NumInstrumentedUnalignedLoads++;
      break;
    case UseDefKind::AlignedOrUnaligned:
      instrumentAlignedOrUnalignedLoad(InsertPoint, LoadAddr, DefIDs, Size);
      NumInstrumentedAlignedOrUnalignedLoads++;
      break;
    case UseDefKind::CondAligned:
      instrumentCondAlignedLoad(InsertPoint, LoadAddr, DefIDs, Size);
      NumInstrumentedCondAlignedLoads++;
      break;
    case UseDefKind::CondUnaligned:
      instrumentCondUnalignedLoad(InsertPoint, LoadAddr, DefIDs, Size);
      NumInstrumentedCondUnalignedLoads++;
      break;
    case UseDefKind::CondAlignedOrUnaligned:
      instrumentCondAlignedOrUnalignedLoad(InsertPoint, LoadAddr, DefIDs, Size);
      NumInstrumentedCondAlignedOrUnalignedLoads++;
      break;
    }
  } else {
    switch(Kind) {
    case UseDefKind::Aligned:
      Builder->CreateCall(AlignedLoadNFn, NArgs);
      NumInstrumentedAlignedLoads++;
      break;
    case UseDefKind::Unaligned:
      Builder->CreateCall(UnalignedLoadNFn, NArgs);
      NumInstrumentedUnalignedLoads++;
      break;
    case UseDefKind::AlignedOrUnaligned:
      Builder->CreateCall(AlignedOrUnalignedLoadNFn, NArgs);
      NumInstrumentedAlignedOrUnalignedLoads++;
      break;
    case UseDefKind::CondAligned:
      Builder->CreateCall(CondAlignedLoadNFn, NArgs);
      NumInstrumentedCondAlignedLoads++;
      break;
    case UseDefKind::CondUnaligned:
      Builder->CreateCall(CondUnalignedLoadNFn, NArgs);
      NumInstrumentedCondUnalignedLoads++;
      break;
    case UseDefKind::CondAlignedOrUnaligned:
      Builder->CreateCall(CondAlignedOrUnalignedLoadNFn, NArgs);
      NumInstrumentedCondAlignedOrUnalignedLoads++;
      break;
    }
  }
}

void DataFlowIntegritySanitizerPass::createDfiLoadStoreFn(SVF::DefID DefID, Value *Target, unsigned Size, ValueVector &DefIDs, UseDefKind Kind, Instruction *InsertPoint) {
  Value *SizeVal = ConstantInt::get(Int64Ty, Size, false);
  createDfiLoadStoreFn(DefID, Target, SizeVal, DefIDs, Kind, InsertPoint);
}

void DataFlowIntegritySanitizerPass::createDfiLoadStoreFn(SVF::DefID DefID, Value *Target, Value *SizeVal, ValueVector &DefIDs, UseDefKind Kind, Instruction *InsertPoint) {
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
  Value *TargetAddr = Builder->CreatePtrToInt(Target, PtrTy);
  Value *DefIDVal = ConstantInt::get(Int16Ty, DefID);
  Value *Argc = ConstantInt::get(Int16Ty, DefIDs.size(), false);

  ValueVector Args{TargetAddr, DefIDVal, Argc};
  Args.append(DefIDs);
  ValueVector NArgs{TargetAddr, DefIDVal, SizeArg, Argc};
  NArgs.append(DefIDs);

  if (ClCheckWithCall) {
    switch(Kind) {
    case UseDefKind::Aligned:
      switch(Size) {
      case 1:  Builder->CreateCall(AlignedLoadStore1Fn, Args);  break;
      case 2:  Builder->CreateCall(AlignedLoadStore2Fn, Args);  break;
      case 4:  Builder->CreateCall(AlignedLoadStore4Fn, Args);  break;
      case 8:  Builder->CreateCall(AlignedLoadStore8Fn, Args);  break;
      case 16: Builder->CreateCall(AlignedLoadStore16Fn, Args); break;
      default: Builder->CreateCall(AlignedLoadStoreNFn, NArgs); break;
      }
      NumInstrumentedAlignedStores++;
      break;
    case UseDefKind::Unaligned:
      switch(Size) {
      case 1:  Builder->CreateCall(UnalignedLoadStore1Fn, Args);  break;
      case 2:  Builder->CreateCall(UnalignedLoadStore2Fn, Args);  break;
      case 4:  Builder->CreateCall(UnalignedLoadStore4Fn, Args);  break;
      case 8:  Builder->CreateCall(UnalignedLoadStore8Fn, Args);  break;
      case 16: Builder->CreateCall(UnalignedLoadStore16Fn, Args); break;
      default: Builder->CreateCall(UnalignedLoadStoreNFn, NArgs); break;
      }
      NumInstrumentedUnalignedStores++;
      break;
    case UseDefKind::AlignedOrUnaligned:
      switch(Size) {
      case 1:  Builder->CreateCall(AlignedOrUnalignedLoadStore1Fn, Args);  break;
      case 2:  Builder->CreateCall(AlignedOrUnalignedLoadStore2Fn, Args);  break;
      case 4:  Builder->CreateCall(AlignedOrUnalignedLoadStore4Fn, Args);  break;
      case 8:  Builder->CreateCall(AlignedOrUnalignedLoadStore8Fn, Args);  break;
      case 16: Builder->CreateCall(AlignedOrUnalignedLoadStore16Fn, Args); break;
      default: Builder->CreateCall(AlignedOrUnalignedLoadStoreNFn, NArgs); break;
      }
      NumInstrumentedAlignedOrUnalignedStores++;
      break;
    case UseDefKind::CondAligned:
      switch(Size) {
      case 1:  Builder->CreateCall(CondAlignedLoadStore1Fn, Args);  break;
      case 2:  Builder->CreateCall(CondAlignedLoadStore2Fn, Args);  break;
      case 4:  Builder->CreateCall(CondAlignedLoadStore4Fn, Args);  break;
      case 8:  Builder->CreateCall(CondAlignedLoadStore8Fn, Args);  break;
      case 16: Builder->CreateCall(CondAlignedLoadStore16Fn, Args); break;
      default: Builder->CreateCall(CondAlignedLoadStoreNFn, NArgs); break;
      }
      NumInstrumentedCondAlignedStores++;
      break;
    case UseDefKind::CondUnaligned:
      switch(Size) {
      case 1:  Builder->CreateCall(CondUnalignedLoadStore1Fn, Args);  break;
      case 2:  Builder->CreateCall(CondUnalignedLoadStore2Fn, Args);  break;
      case 4:  Builder->CreateCall(CondUnalignedLoadStore4Fn, Args);  break;
      case 8:  Builder->CreateCall(CondUnalignedLoadStore8Fn, Args);  break;
      case 16: Builder->CreateCall(CondUnalignedLoadStore16Fn, Args); break;
      default: Builder->CreateCall(CondUnalignedLoadStoreNFn, NArgs); break;
      }
      NumInstrumentedCondUnalignedStores++;
      break;
    case UseDefKind::CondAlignedOrUnaligned:
      switch(Size) {
      case 1:  Builder->CreateCall(CondAlignedOrUnalignedLoadStore1Fn, Args);  break;
      case 2:  Builder->CreateCall(CondAlignedOrUnalignedLoadStore2Fn, Args);  break;
      case 4:  Builder->CreateCall(CondAlignedOrUnalignedLoadStore4Fn, Args);  break;
      case 8:  Builder->CreateCall(CondAlignedOrUnalignedLoadStore8Fn, Args);  break;
      case 16: Builder->CreateCall(CondAlignedOrUnalignedLoadStore16Fn, Args); break;
      default: Builder->CreateCall(CondAlignedOrUnalignedLoadStoreNFn, NArgs); break;
      }
      NumInstrumentedCondAlignedOrUnalignedStores++;
      break;
    }
  } else if (inlineCheckCode(Size)) {
    switch(Kind) {
    case UseDefKind::Aligned:
      instrumentAlignedRaceStore(InsertPoint, TargetAddr, DefIDVal, DefIDs, Size);
      NumInstrumentedAlignedStores++;
      break;
    case UseDefKind::Unaligned:
      instrumentUnalignedRaceStore(InsertPoint, TargetAddr, DefIDVal, DefIDs, Size);
      NumInstrumentedUnalignedStores++;
      break;
    case UseDefKind::AlignedOrUnaligned:
      instrumentAlignedOrUnalignedRaceStore(InsertPoint, TargetAddr, DefIDVal, DefIDs, Size);
      NumInstrumentedAlignedOrUnalignedStores++;
      break;
    case UseDefKind::CondAligned:
      instrumentCondAlignedRaceStore(InsertPoint, TargetAddr, DefIDVal, DefIDs, Size);
      NumInstrumentedCondAlignedStores++;
      break;
    case UseDefKind::CondUnaligned:
      instrumentCondUnalignedRaceStore(InsertPoint, TargetAddr, DefIDVal, DefIDs, Size);
      NumInstrumentedCondUnalignedStores++;
      break;
    case UseDefKind::CondAlignedOrUnaligned:
      instrumentCondAlignedOrUnalignedRaceStore(InsertPoint, TargetAddr, DefIDVal, DefIDs, Size);
      NumInstrumentedCondAlignedOrUnalignedStores++;
      break;
    }
  } else {
    switch(Kind) {
    case UseDefKind::Aligned:
      Builder->CreateCall(AlignedLoadStoreNFn, NArgs);
      NumInstrumentedAlignedStores++;
      break;
    case UseDefKind::Unaligned:
      Builder->CreateCall(UnalignedLoadStoreNFn, NArgs);
      NumInstrumentedUnalignedStores++;
      break;
    case UseDefKind::AlignedOrUnaligned:
      Builder->CreateCall(AlignedOrUnalignedLoadStoreNFn, NArgs);
      NumInstrumentedAlignedOrUnalignedStores++;
      break;
    case UseDefKind::CondAligned:
      Builder->CreateCall(CondAlignedLoadStoreNFn, NArgs);
      NumInstrumentedCondAlignedStores++;
      break;
    case UseDefKind::CondUnaligned:
      Builder->CreateCall(CondUnalignedLoadStoreNFn, NArgs);
      NumInstrumentedCondUnalignedStores++;
      break;
    case UseDefKind::CondAlignedOrUnaligned:
      Builder->CreateCall(CondAlignedOrUnalignedLoadStoreNFn, NArgs);
      NumInstrumentedCondAlignedOrUnalignedStores++;
      break;
    }
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
