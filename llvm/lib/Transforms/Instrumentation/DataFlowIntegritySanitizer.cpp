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
  Type *DefIDTy = DefID->getType();
  Type *PtrTy = PointerType::getInt16PtrTy(IRB->getContext());
  Value *ShadowPtr = IRB->CreateIntToPtr(ShadowAddr, PtrTy);
  return IRB->CreateStore(DefID, ShadowPtr);
}

/// Aligned store
void DataFlowIntegritySanitizerPass::instrumentAlignedStore4(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Value *AlignedAddr = AlignAddr(Builder.get(), StoreAddr);
  Value *ShadowAddr = AlignedMemToShadow(Builder.get(), AlignedAddr);
  setDefID(Builder.get(), ShadowAddr, DefID);
}
void DataFlowIntegritySanitizerPass::instrumentAlignedStore8(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Value *ShadowAddr0 = AlignedMemToShadow(Builder.get(), StoreAddr);
  Value *ShadowAddr1 = getNextShadowAddr(Builder.get(), ShadowAddr0, 1);
  setDefID(Builder.get(), ShadowAddr0, DefID);
  setDefID(Builder.get(), ShadowAddr1, DefID);
}
void DataFlowIntegritySanitizerPass::instrumentAlignedStore16(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Value *ShadowAddr0 = AlignedMemToShadow(Builder.get(), StoreAddr);
  Value *ShadowAddr1 = getNextShadowAddr(Builder.get(), ShadowAddr0, 1);
  Value *ShadowAddr2 = getNextShadowAddr(Builder.get(), ShadowAddr0, 2);
  Value *ShadowAddr3 = getNextShadowAddr(Builder.get(), ShadowAddr0, 3);
  setDefID(Builder.get(), ShadowAddr0, DefID);
  setDefID(Builder.get(), ShadowAddr1, DefID);
  setDefID(Builder.get(), ShadowAddr2, DefID);
  setDefID(Builder.get(), ShadowAddr3, DefID);
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
void DataFlowIntegritySanitizerPass::instrumentUnalignedStore1(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Value *ShadowAddr = UnalignedMemToShadow(Builder.get(), StoreAddr);
  setDefID(Builder.get(), ShadowAddr, DefID);
}
void DataFlowIntegritySanitizerPass::instrumentUnalignedStore2(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Value *ShadowAddr0 = UnalignedMemToShadow(Builder.get(), StoreAddr);
  Value *ShadowAddr1 = getNextShadowAddr(Builder.get(), ShadowAddr0, 1);
  setDefID(Builder.get(), ShadowAddr0, DefID);
  setDefID(Builder.get(), ShadowAddr1, DefID);
}
void DataFlowIntegritySanitizerPass::instrumentUnalignedStore4(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Value *ShadowAddr0 = UnalignedMemToShadow(Builder.get(), StoreAddr);
  Value *ShadowAddr1 = getNextShadowAddr(Builder.get(), ShadowAddr0, 1);
  Value *ShadowAddr2 = getNextShadowAddr(Builder.get(), ShadowAddr0, 2);
  Value *ShadowAddr3 = getNextShadowAddr(Builder.get(), ShadowAddr0, 3);
  setDefID(Builder.get(), ShadowAddr0, DefID);
  setDefID(Builder.get(), ShadowAddr1, DefID);
  setDefID(Builder.get(), ShadowAddr2, DefID);
  setDefID(Builder.get(), ShadowAddr3, DefID);
}
void DataFlowIntegritySanitizerPass::instrumentUnalignedStore8(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Value *ShadowAddr0 = UnalignedMemToShadow(Builder.get(), StoreAddr);
  Value *ShadowAddr1 = getNextShadowAddr(Builder.get(), ShadowAddr0, 1);
  Value *ShadowAddr2 = getNextShadowAddr(Builder.get(), ShadowAddr0, 2);
  Value *ShadowAddr3 = getNextShadowAddr(Builder.get(), ShadowAddr0, 3);
  Value *ShadowAddr4 = getNextShadowAddr(Builder.get(), ShadowAddr0, 4);
  Value *ShadowAddr5 = getNextShadowAddr(Builder.get(), ShadowAddr0, 5);
  Value *ShadowAddr6 = getNextShadowAddr(Builder.get(), ShadowAddr0, 6);
  Value *ShadowAddr7 = getNextShadowAddr(Builder.get(), ShadowAddr0, 7);
  setDefID(Builder.get(), ShadowAddr0, DefID);
  setDefID(Builder.get(), ShadowAddr1, DefID);
  setDefID(Builder.get(), ShadowAddr2, DefID);
  setDefID(Builder.get(), ShadowAddr3, DefID);
  setDefID(Builder.get(), ShadowAddr4, DefID);
  setDefID(Builder.get(), ShadowAddr5, DefID);
  setDefID(Builder.get(), ShadowAddr6, DefID);
  setDefID(Builder.get(), ShadowAddr7, DefID);
}
void DataFlowIntegritySanitizerPass::instrumentUnalignedStore16(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Value *ShadowAddr0  = UnalignedMemToShadow(Builder.get(), StoreAddr);
  Value *ShadowAddr1  = getNextShadowAddr(Builder.get(), ShadowAddr0, 1);
  Value *ShadowAddr2  = getNextShadowAddr(Builder.get(), ShadowAddr0, 2);
  Value *ShadowAddr3  = getNextShadowAddr(Builder.get(), ShadowAddr0, 3);
  Value *ShadowAddr4  = getNextShadowAddr(Builder.get(), ShadowAddr0, 4);
  Value *ShadowAddr5  = getNextShadowAddr(Builder.get(), ShadowAddr0, 5);
  Value *ShadowAddr6  = getNextShadowAddr(Builder.get(), ShadowAddr0, 6);
  Value *ShadowAddr7  = getNextShadowAddr(Builder.get(), ShadowAddr0, 7);
  Value *ShadowAddr8  = getNextShadowAddr(Builder.get(), ShadowAddr0, 8);
  Value *ShadowAddr9  = getNextShadowAddr(Builder.get(), ShadowAddr0, 9);
  Value *ShadowAddr10 = getNextShadowAddr(Builder.get(), ShadowAddr0, 10);
  Value *ShadowAddr11 = getNextShadowAddr(Builder.get(), ShadowAddr0, 11);
  Value *ShadowAddr12 = getNextShadowAddr(Builder.get(), ShadowAddr0, 12);
  Value *ShadowAddr13 = getNextShadowAddr(Builder.get(), ShadowAddr0, 13);
  Value *ShadowAddr14 = getNextShadowAddr(Builder.get(), ShadowAddr0, 14);
  Value *ShadowAddr15 = getNextShadowAddr(Builder.get(), ShadowAddr0, 15);
  setDefID(Builder.get(), ShadowAddr0, DefID);
  setDefID(Builder.get(), ShadowAddr1, DefID);
  setDefID(Builder.get(), ShadowAddr2, DefID);
  setDefID(Builder.get(), ShadowAddr3, DefID);
  setDefID(Builder.get(), ShadowAddr4, DefID);
  setDefID(Builder.get(), ShadowAddr5, DefID);
  setDefID(Builder.get(), ShadowAddr6, DefID);
  setDefID(Builder.get(), ShadowAddr7, DefID);
  setDefID(Builder.get(), ShadowAddr8, DefID);
  setDefID(Builder.get(), ShadowAddr9, DefID);
  setDefID(Builder.get(), ShadowAddr10, DefID);
  setDefID(Builder.get(), ShadowAddr11, DefID);
  setDefID(Builder.get(), ShadowAddr12, DefID);
  setDefID(Builder.get(), ShadowAddr13, DefID);
  setDefID(Builder.get(), ShadowAddr14, DefID);
  setDefID(Builder.get(), ShadowAddr15, DefID);
}

/// Aligned or unaligned store
void DataFlowIntegritySanitizerPass::instrumentAlignedOrUnalignedStore1(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Instruction *Then, *Else;
  insertIfAddrIsInAlignedRegion(Builder.get(), StoreAddr, InsertPoint, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentAlignedStore4(Then, StoreAddr, DefID);
  Builder->SetInsertPoint(Else);
  instrumentUnalignedStore1(Else, StoreAddr, DefID);
}
void DataFlowIntegritySanitizerPass::instrumentAlignedOrUnalignedStore2(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Instruction *Then, *Else;
  insertIfAddrIsInAlignedRegion(Builder.get(), StoreAddr, InsertPoint, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentAlignedStore4(Then, StoreAddr, DefID);
  Builder->SetInsertPoint(Else);
  instrumentUnalignedStore2(Else, StoreAddr, DefID);
}
void DataFlowIntegritySanitizerPass::instrumentAlignedOrUnalignedStore4(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Instruction *Then, *Else;
  insertIfAddrIsInAlignedRegion(Builder.get(), StoreAddr, InsertPoint, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentAlignedStore4(Then, StoreAddr, DefID);
  Builder->SetInsertPoint(Else);
  instrumentUnalignedStore4(Else, StoreAddr, DefID);
}
void DataFlowIntegritySanitizerPass::instrumentAlignedOrUnalignedStore8(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Instruction *Then, *Else;
  insertIfAddrIsInAlignedRegion(Builder.get(), StoreAddr, InsertPoint, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentAlignedStore8(Then, StoreAddr, DefID);
  Builder->SetInsertPoint(Else);
  instrumentUnalignedStore8(Else, StoreAddr, DefID);
}
void DataFlowIntegritySanitizerPass::instrumentAlignedOrUnalignedStore16(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Instruction *Then, *Else;
  insertIfAddrIsInAlignedRegion(Builder.get(), StoreAddr, InsertPoint, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentAlignedStore16(Then, StoreAddr, DefID);
  Builder->SetInsertPoint(Else);
  instrumentUnalignedStore16(Else, StoreAddr, DefID);
}
void DataFlowIntegritySanitizerPass::instrumentAlignedOrUnalignedStoreN(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  // TODO
}

/// Conditional aligned
void DataFlowIntegritySanitizerPass::instrumentCondAlignedStore1(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  instrumentCondAlignedStore4(InsertPoint, StoreAddr, DefID);
}
void DataFlowIntegritySanitizerPass::instrumentCondAlignedStore2(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  instrumentCondAlignedStore4(InsertPoint, StoreAddr, DefID);
}
void DataFlowIntegritySanitizerPass::instrumentCondAlignedStore4(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Instruction *Then, *Else;
  insertIfAddrIsInAlignedRegion(Builder.get(), StoreAddr, InsertPoint, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentAlignedStore4(Then, StoreAddr, DefID);
}
void DataFlowIntegritySanitizerPass::instrumentCondAlignedStore8(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Instruction *Then, *Else;
  insertIfAddrIsInAlignedRegion(Builder.get(), StoreAddr, InsertPoint, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentAlignedStore8(Then, StoreAddr, DefID);
}
void DataFlowIntegritySanitizerPass::instrumentCondAlignedStore16(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Instruction *Then, *Else;
  insertIfAddrIsInAlignedRegion(Builder.get(), StoreAddr, InsertPoint, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentAlignedStore16(Then, StoreAddr, DefID);
}

/// Conditional unaligned
void DataFlowIntegritySanitizerPass::instrumentCondUnalignedStore1(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Instruction *Then, *Else;
  insertIfAddrIsInUnalignedRegion(Builder.get(), StoreAddr, InsertPoint, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentUnalignedStore1(Then, StoreAddr, DefID);
}
void DataFlowIntegritySanitizerPass::instrumentCondUnalignedStore2(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Instruction *Then, *Else;
  insertIfAddrIsInUnalignedRegion(Builder.get(), StoreAddr, InsertPoint, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentUnalignedStore2(Then, StoreAddr, DefID);
}
void DataFlowIntegritySanitizerPass::instrumentCondUnalignedStore4(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Instruction *Then, *Else;
  insertIfAddrIsInUnalignedRegion(Builder.get(), StoreAddr, InsertPoint, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentUnalignedStore4(Then, StoreAddr, DefID);
}
void DataFlowIntegritySanitizerPass::instrumentCondUnalignedStore8(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Instruction *Then, *Else;
  insertIfAddrIsInUnalignedRegion(Builder.get(), StoreAddr, InsertPoint, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentUnalignedStore8(Then, StoreAddr, DefID);
}
void DataFlowIntegritySanitizerPass::instrumentCondUnalignedStore16(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Instruction *Then, *Else;
  insertIfAddrIsInUnalignedRegion(Builder.get(), StoreAddr, InsertPoint, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentUnalignedStore16(Then, StoreAddr, DefID);
}

/// Conditional aligned or unaligned
void DataFlowIntegritySanitizerPass::instrumentCondAlignedOrUnalignedStore1(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Instruction *Then1, *Then2, *Else;
  insertIfAddrIsInAlignedRegionElseIfAddrIsInUnalignedRegion(Builder.get(), StoreAddr, InsertPoint, &Then1, &Then2, &Else);
  Builder->SetInsertPoint(Then1);
  instrumentAlignedStore4(Then1, StoreAddr, DefID);
  Builder->SetInsertPoint(Then2);
  instrumentUnalignedStore1(Then2, StoreAddr, DefID);
}
void DataFlowIntegritySanitizerPass::instrumentCondAlignedOrUnalignedStore2(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Instruction *Then1, *Then2, *Else;
  insertIfAddrIsInAlignedRegionElseIfAddrIsInUnalignedRegion(Builder.get(), StoreAddr, InsertPoint, &Then1, &Then2, &Else);
  Builder->SetInsertPoint(Then1);
  instrumentAlignedStore4(Then1, StoreAddr, DefID);
  Builder->SetInsertPoint(Then2);
  instrumentUnalignedStore2(Then2, StoreAddr, DefID);
}
void DataFlowIntegritySanitizerPass::instrumentCondAlignedOrUnalignedStore4(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Instruction *Then1, *Then2, *Else;
  insertIfAddrIsInAlignedRegionElseIfAddrIsInUnalignedRegion(Builder.get(), StoreAddr, InsertPoint, &Then1, &Then2, &Else);
  Builder->SetInsertPoint(Then1);
  instrumentAlignedStore4(Then1, StoreAddr, DefID);
  Builder->SetInsertPoint(Then2);
  instrumentUnalignedStore4(Then2, StoreAddr, DefID);
}
void DataFlowIntegritySanitizerPass::instrumentCondAlignedOrUnalignedStore8(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Instruction *Then1, *Then2, *Else;
  insertIfAddrIsInAlignedRegionElseIfAddrIsInUnalignedRegion(Builder.get(), StoreAddr, InsertPoint, &Then1, &Then2, &Else);
  Builder->SetInsertPoint(Then1);
  instrumentAlignedStore8(Then1, StoreAddr, DefID);
  Builder->SetInsertPoint(Then2);
  instrumentUnalignedStore8(Then2, StoreAddr, DefID);
}
void DataFlowIntegritySanitizerPass::instrumentCondAlignedOrUnalignedStore16(Instruction *InsertPoint, Value *StoreAddr, Value *DefID) {
  Instruction *Then1, *Then2, *Else;
  insertIfAddrIsInAlignedRegionElseIfAddrIsInUnalignedRegion(Builder.get(), StoreAddr, InsertPoint, &Then1, &Then2, &Else);
  Builder->SetInsertPoint(Then1);
  instrumentAlignedStore16(Then1, StoreAddr, DefID);
  Builder->SetInsertPoint(Then2);
  instrumentUnalignedStore16(Then2, StoreAddr, DefID);
}



/* --- Check functions --- */
/// Get DefID
static Value *getDefID(IRBuilder<> *IRB, Value *ShadowAddr) {
  Type *DefIDTy = IRB->getInt16Ty();
  Type *PtrTy = PointerType::getInt16PtrTy(IRB->getContext());
  Value *ShadowPtr = IRB->CreateIntToPtr(ShadowAddr, PtrTy);
  return IRB->CreateLoad(DefIDTy, ShadowPtr);
}
static Value *getNextDefID(IRBuilder<> *IRB, Value *ShadowAddr, unsigned Num) {
  Value *NextShadowAddr = getNextShadowAddr(IRB, ShadowAddr, Num);
  return getDefID(IRB, NextShadowAddr);
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

/// Insert if-then to report error
void DataFlowIntegritySanitizerPass::insertIfThenAndErrorReport(Value *Cond, Instruction *InsertPoint, Value *LoadAddr, ValueVector &DefIDs) {
  Instruction *CrashTerm = SplitBlockAndInsertIfThen(Cond, InsertPoint, /* unreachable */ true);
  Instruction *Crash = generateCrashCode(CrashTerm, LoadAddr, DefIDs);
  Crash->setDebugLoc(InsertPoint->getDebugLoc());
}

/// Aligned check
void DataFlowIntegritySanitizerPass::instrumentAlignedLoad4(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Value *AlignedAddr = AlignAddr(Builder.get(), LoadAddr);
  Value *ShadowAddr = AlignedMemToShadow(Builder.get(), AlignedAddr);
  Value *CurrDefID = getDefID(Builder.get(), ShadowAddr);
  Value *IsAttack = checkDefIDs(Builder.get(), CurrDefID, DefIDs);
  insertIfThenAndErrorReport(IsAttack, Load, LoadAddr, DefIDs);
}
void DataFlowIntegritySanitizerPass::instrumentAlignedLoad8(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Value *ShadowAddr = AlignedMemToShadow(Builder.get(), LoadAddr);
  Value *CurrDefID0 = getDefID(Builder.get(), ShadowAddr);
  Value *CurrDefID1 = getNextDefID(Builder.get(), ShadowAddr, 1);
  Value *IsAttack0  = checkDefIDs(Builder.get(), CurrDefID0, DefIDs);
  Value *IsAttack1  = checkDefIDs(Builder.get(), CurrDefID1, DefIDs);
  Value *IsAttack   = Builder->CreateLogicalOr(IsAttack0, IsAttack1);
  insertIfThenAndErrorReport(IsAttack, Load, LoadAddr, DefIDs);
}
void DataFlowIntegritySanitizerPass::instrumentAlignedLoad16(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Value *ShadowAddr = AlignedMemToShadow(Builder.get(), LoadAddr);
  Value *CurrDefID0 = getDefID(Builder.get(), ShadowAddr);
  Value *CurrDefID1 = getNextDefID(Builder.get(), ShadowAddr, 1);
  Value *CurrDefID2 = getNextDefID(Builder.get(), ShadowAddr, 2);
  Value *CurrDefID3 = getNextDefID(Builder.get(), ShadowAddr, 3);
  Value *IsAttack0  = checkDefIDs(Builder.get(), CurrDefID0, DefIDs);
  Value *IsAttack1  = checkDefIDs(Builder.get(), CurrDefID1, DefIDs);
  Value *IsAttack2  = checkDefIDs(Builder.get(), CurrDefID2, DefIDs);
  Value *IsAttack3  = checkDefIDs(Builder.get(), CurrDefID3, DefIDs);
  Value *IsAttack0_1 = Builder->CreateLogicalOr(IsAttack0, IsAttack1);
  Value *IsAttack2_3 = Builder->CreateLogicalOr(IsAttack2, IsAttack3);
  Value *IsAttack    = Builder->CreateLogicalOr(IsAttack0_1, IsAttack2_3);
  insertIfThenAndErrorReport(IsAttack, Load, LoadAddr, DefIDs);
}
void DataFlowIntegritySanitizerPass::instrumentAlignedLoadN(Instruction *Load, Value *LoadAddr, unsigned Size, ValueVector &DefIDs) {
  // TODO
}

/// Unaligned check
void DataFlowIntegritySanitizerPass::instrumentUnalignedLoad1(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Value *ShadowAddr = UnalignedMemToShadow(Builder.get(), LoadAddr);
  Value *CurrDefID  = getDefID(Builder.get(), ShadowAddr);
  Value *IsAttack = checkDefIDs(Builder.get(), CurrDefID, DefIDs);
  insertIfThenAndErrorReport(IsAttack, Load, LoadAddr, DefIDs);
}
void DataFlowIntegritySanitizerPass::instrumentUnalignedLoad2(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Value *ShadowAddr = UnalignedMemToShadow(Builder.get(), LoadAddr);
  Value *CurrDefID0 = getDefID(Builder.get(), ShadowAddr);
  Value *CurrDefID1 = getNextDefID(Builder.get(), ShadowAddr, 1);
  Value *IsAttack0 = checkDefIDs(Builder.get(), CurrDefID0, DefIDs);
  Value *IsAttack1 = checkDefIDs(Builder.get(), CurrDefID1, DefIDs);
  Value *IsAttack  = Builder->CreateLogicalOr(IsAttack0, IsAttack1);
  insertIfThenAndErrorReport(IsAttack, Load, LoadAddr, DefIDs);
}
void DataFlowIntegritySanitizerPass::instrumentUnalignedLoad4(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Value *ShadowAddr = UnalignedMemToShadow(Builder.get(), LoadAddr);
  Value *CurrDefID0 = getDefID(Builder.get(), ShadowAddr);
  Value *CurrDefID1 = getNextDefID(Builder.get(), ShadowAddr, 1);
  Value *CurrDefID2 = getNextDefID(Builder.get(), ShadowAddr, 2);
  Value *CurrDefID3 = getNextDefID(Builder.get(), ShadowAddr, 3);
  Value *IsAttack0 = checkDefIDs(Builder.get(), CurrDefID0, DefIDs);
  Value *IsAttack1 = checkDefIDs(Builder.get(), CurrDefID1, DefIDs);
  Value *IsAttack2 = checkDefIDs(Builder.get(), CurrDefID2, DefIDs);
  Value *IsAttack3 = checkDefIDs(Builder.get(), CurrDefID3, DefIDs);
  Value *IsAttack01 = Builder->CreateLogicalOr(IsAttack0, IsAttack1);
  Value *IsAttack23 = Builder->CreateLogicalOr(IsAttack2, IsAttack3);
  Value *IsAttack   = Builder->CreateLogicalOr(IsAttack01, IsAttack23);
  insertIfThenAndErrorReport(IsAttack, Load, LoadAddr, DefIDs);
}
void DataFlowIntegritySanitizerPass::instrumentUnalignedLoad8(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Value *ShadowAddr = UnalignedMemToShadow(Builder.get(), LoadAddr);
  Value *CurrDefID0 = getDefID(Builder.get(), ShadowAddr);
  Value *CurrDefID1 = getNextDefID(Builder.get(), ShadowAddr, 1);
  Value *CurrDefID2 = getNextDefID(Builder.get(), ShadowAddr, 2);
  Value *CurrDefID3 = getNextDefID(Builder.get(), ShadowAddr, 3);
  Value *CurrDefID4 = getNextDefID(Builder.get(), ShadowAddr, 4);
  Value *CurrDefID5 = getNextDefID(Builder.get(), ShadowAddr, 5);
  Value *CurrDefID6 = getNextDefID(Builder.get(), ShadowAddr, 6);
  Value *CurrDefID7 = getNextDefID(Builder.get(), ShadowAddr, 7);
  Value *IsAttack0 = checkDefIDs(Builder.get(), CurrDefID0, DefIDs);
  Value *IsAttack1 = checkDefIDs(Builder.get(), CurrDefID1, DefIDs);
  Value *IsAttack2 = checkDefIDs(Builder.get(), CurrDefID2, DefIDs);
  Value *IsAttack3 = checkDefIDs(Builder.get(), CurrDefID3, DefIDs);
  Value *IsAttack4 = checkDefIDs(Builder.get(), CurrDefID4, DefIDs);
  Value *IsAttack5 = checkDefIDs(Builder.get(), CurrDefID5, DefIDs);
  Value *IsAttack6 = checkDefIDs(Builder.get(), CurrDefID6, DefIDs);
  Value *IsAttack7 = checkDefIDs(Builder.get(), CurrDefID7, DefIDs);
  Value *IsAttack01 = Builder->CreateLogicalOr(IsAttack0, IsAttack1);
  Value *IsAttack23 = Builder->CreateLogicalOr(IsAttack2, IsAttack3);
  Value *IsAttack45 = Builder->CreateLogicalOr(IsAttack4, IsAttack5);
  Value *IsAttack67 = Builder->CreateLogicalOr(IsAttack6, IsAttack7);
  Value *IsAttack0123 = Builder->CreateLogicalOr(IsAttack01, IsAttack23);
  Value *IsAttack4567 = Builder->CreateLogicalOr(IsAttack45, IsAttack67);
  Value *IsAttack  = Builder->CreateLogicalOr(IsAttack0123, IsAttack4567);
  insertIfThenAndErrorReport(IsAttack, Load, LoadAddr, DefIDs);
}
void DataFlowIntegritySanitizerPass::instrumentUnalignedLoad16(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Value *ShadowAddr = UnalignedMemToShadow(Builder.get(), LoadAddr);
  Value *CurrDefID0 = getDefID(Builder.get(), ShadowAddr);
  Value *CurrDefID1 = getNextDefID(Builder.get(), ShadowAddr, 1);
  Value *CurrDefID2 = getNextDefID(Builder.get(), ShadowAddr, 2);
  Value *CurrDefID3 = getNextDefID(Builder.get(), ShadowAddr, 3);
  Value *CurrDefID4 = getNextDefID(Builder.get(), ShadowAddr, 4);
  Value *CurrDefID5 = getNextDefID(Builder.get(), ShadowAddr, 5);
  Value *CurrDefID6 = getNextDefID(Builder.get(), ShadowAddr, 6);
  Value *CurrDefID7 = getNextDefID(Builder.get(), ShadowAddr, 7);
  Value *CurrDefID8 = getNextDefID(Builder.get(), ShadowAddr, 8);
  Value *CurrDefID9 = getNextDefID(Builder.get(), ShadowAddr, 9);
  Value *CurrDefID10 = getNextDefID(Builder.get(), ShadowAddr, 10);
  Value *CurrDefID11 = getNextDefID(Builder.get(), ShadowAddr, 11);
  Value *CurrDefID12 = getNextDefID(Builder.get(), ShadowAddr, 12);
  Value *CurrDefID13 = getNextDefID(Builder.get(), ShadowAddr, 13);
  Value *CurrDefID14 = getNextDefID(Builder.get(), ShadowAddr, 14);
  Value *CurrDefID15 = getNextDefID(Builder.get(), ShadowAddr, 15);
  Value *IsAttack0 = checkDefIDs(Builder.get(), CurrDefID0, DefIDs);
  Value *IsAttack1 = checkDefIDs(Builder.get(), CurrDefID1, DefIDs);
  Value *IsAttack2 = checkDefIDs(Builder.get(), CurrDefID2, DefIDs);
  Value *IsAttack3 = checkDefIDs(Builder.get(), CurrDefID3, DefIDs);
  Value *IsAttack4 = checkDefIDs(Builder.get(), CurrDefID4, DefIDs);
  Value *IsAttack5 = checkDefIDs(Builder.get(), CurrDefID5, DefIDs);
  Value *IsAttack6 = checkDefIDs(Builder.get(), CurrDefID6, DefIDs);
  Value *IsAttack7 = checkDefIDs(Builder.get(), CurrDefID7, DefIDs);
  Value *IsAttack8 = checkDefIDs(Builder.get(), CurrDefID8, DefIDs);
  Value *IsAttack9 = checkDefIDs(Builder.get(), CurrDefID9, DefIDs);
  Value *IsAttack10 = checkDefIDs(Builder.get(), CurrDefID10, DefIDs);
  Value *IsAttack11 = checkDefIDs(Builder.get(), CurrDefID11, DefIDs);
  Value *IsAttack12 = checkDefIDs(Builder.get(), CurrDefID12, DefIDs);
  Value *IsAttack13 = checkDefIDs(Builder.get(), CurrDefID13, DefIDs);
  Value *IsAttack14 = checkDefIDs(Builder.get(), CurrDefID14, DefIDs);
  Value *IsAttack15 = checkDefIDs(Builder.get(), CurrDefID15, DefIDs);
  Value *IsAttack01 = Builder->CreateLogicalOr(IsAttack0, IsAttack1);
  Value *IsAttack23 = Builder->CreateLogicalOr(IsAttack2, IsAttack3);
  Value *IsAttack45 = Builder->CreateLogicalOr(IsAttack4, IsAttack5);
  Value *IsAttack67 = Builder->CreateLogicalOr(IsAttack6, IsAttack7);
  Value *IsAttack89 = Builder->CreateLogicalOr(IsAttack8, IsAttack9);
  Value *IsAttack1011 = Builder->CreateLogicalOr(IsAttack10, IsAttack11);
  Value *IsAttack1213 = Builder->CreateLogicalOr(IsAttack12, IsAttack13);
  Value *IsAttack1415 = Builder->CreateLogicalOr(IsAttack14, IsAttack15);
  Value *IsAttack0123 = Builder->CreateLogicalOr(IsAttack01, IsAttack23);
  Value *IsAttack4567 = Builder->CreateLogicalOr(IsAttack45, IsAttack67);
  Value *IsAttack891011 = Builder->CreateLogicalOr(IsAttack89, IsAttack1011);
  Value *IsAttack12131415 = Builder->CreateLogicalOr(IsAttack1213, IsAttack1415);
  Value *IsAttack01234567 = Builder->CreateLogicalOr(IsAttack0123, IsAttack4567);
  Value *IsAttack891011121314 = Builder->CreateLogicalOr(IsAttack891011, IsAttack12131415);
  Value *IsAttack  = Builder->CreateLogicalOr(IsAttack01234567, IsAttack891011121314);
  insertIfThenAndErrorReport(IsAttack, Load, LoadAddr, DefIDs);
}
void DataFlowIntegritySanitizerPass::instrumentUnalignedLoadN(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  // TODO
}

/// Aligned or unaligned
void DataFlowIntegritySanitizerPass::instrumentAlignedOrUnalignedLoad1(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Instruction *Then, *Else;
  insertIfAddrIsInAlignedRegion(Builder.get(), LoadAddr, Load, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentAlignedLoad4(Then, LoadAddr, DefIDs);
  Builder->SetInsertPoint(Else);
  instrumentUnalignedLoad1(Else, LoadAddr, DefIDs);
}
void DataFlowIntegritySanitizerPass::instrumentAlignedOrUnalignedLoad2(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Instruction *Then, *Else;
  insertIfAddrIsInAlignedRegion(Builder.get(), LoadAddr, Load, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentAlignedLoad4(Then, LoadAddr, DefIDs);
  Builder->SetInsertPoint(Else);
  instrumentUnalignedLoad2(Else, LoadAddr, DefIDs);
}
void DataFlowIntegritySanitizerPass::instrumentAlignedOrUnalignedLoad4(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Instruction *Then, *Else;
  insertIfAddrIsInAlignedRegion(Builder.get(), LoadAddr, Load, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentAlignedLoad4(Then, LoadAddr, DefIDs);
  Builder->SetInsertPoint(Else);
  instrumentUnalignedLoad4(Else, LoadAddr, DefIDs);
}
void DataFlowIntegritySanitizerPass::instrumentAlignedOrUnalignedLoad8(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Instruction *Then, *Else;
  insertIfAddrIsInAlignedRegion(Builder.get(), LoadAddr, Load, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentAlignedLoad8(Then, LoadAddr, DefIDs);
  Builder->SetInsertPoint(Else);
  instrumentUnalignedLoad8(Else, LoadAddr, DefIDs);
}
void DataFlowIntegritySanitizerPass::instrumentAlignedOrUnalignedLoad16(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Instruction *Then, *Else;
  insertIfAddrIsInAlignedRegion(Builder.get(), LoadAddr, Load, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentAlignedLoad16(Then, LoadAddr, DefIDs);
  Builder->SetInsertPoint(Else);
  instrumentUnalignedLoad16(Else, LoadAddr, DefIDs);
}

/// Conditional aligned
void DataFlowIntegritySanitizerPass::instrumentCondAlignedLoad4(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Instruction *Then, *Else;
  insertIfAddrIsInAlignedRegion(Builder.get(), LoadAddr, Load, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentAlignedLoad4(Then, LoadAddr, DefIDs);
}
void DataFlowIntegritySanitizerPass::instrumentCondAlignedLoad8(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Instruction *Then, *Else;
  insertIfAddrIsInAlignedRegion(Builder.get(), LoadAddr, Load, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentAlignedLoad8(Then, LoadAddr, DefIDs);
}
void DataFlowIntegritySanitizerPass::instrumentCondAlignedLoad16(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Instruction *Then, *Else;
  insertIfAddrIsInAlignedRegion(Builder.get(), LoadAddr, Load, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentAlignedLoad16(Then, LoadAddr, DefIDs);
}

/// Conditional unaligned
void DataFlowIntegritySanitizerPass::instrumentCondUnalignedLoad1(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Instruction *Then, *Else;
  insertIfAddrIsInUnalignedRegion(Builder.get(), LoadAddr, Load, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentUnalignedLoad1(Then, LoadAddr, DefIDs);
}
void DataFlowIntegritySanitizerPass::instrumentCondUnalignedLoad2(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Instruction *Then, *Else;
  insertIfAddrIsInUnalignedRegion(Builder.get(), LoadAddr, Load, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentUnalignedLoad2(Then, LoadAddr, DefIDs);
}
void DataFlowIntegritySanitizerPass::instrumentCondUnalignedLoad4(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Instruction *Then, *Else;
  insertIfAddrIsInUnalignedRegion(Builder.get(), LoadAddr, Load, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentUnalignedLoad4(Then, LoadAddr, DefIDs);
}
void DataFlowIntegritySanitizerPass::instrumentCondUnalignedLoad8(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Instruction *Then, *Else;
  insertIfAddrIsInUnalignedRegion(Builder.get(), LoadAddr, Load, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentUnalignedLoad8(Then, LoadAddr, DefIDs);
}
void DataFlowIntegritySanitizerPass::instrumentCondUnalignedLoad16(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Instruction *Then, *Else;
  insertIfAddrIsInUnalignedRegion(Builder.get(), LoadAddr, Load, &Then, &Else);
  Builder->SetInsertPoint(Then);
  instrumentUnalignedLoad16(Then, LoadAddr, DefIDs);
}

/// Conditional aligned or unaligned
void DataFlowIntegritySanitizerPass::instrumentCondAlignedOrUnalignedLoad1(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Instruction *Then1, *Then2, *Else;
  insertIfAddrIsInAlignedRegionElseIfAddrIsInUnalignedRegion(Builder.get(), LoadAddr, Load, &Then1, &Then2, &Else);
  Builder->SetInsertPoint(Then1);
  instrumentAlignedLoad4(Then1, LoadAddr, DefIDs);
  Builder->SetInsertPoint(Then2);
  instrumentUnalignedLoad1(Then2, LoadAddr, DefIDs);
}
void DataFlowIntegritySanitizerPass::instrumentCondAlignedOrUnalignedLoad2(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Instruction *Then1, *Then2, *Else;
  insertIfAddrIsInAlignedRegionElseIfAddrIsInUnalignedRegion(Builder.get(), LoadAddr, Load, &Then1, &Then2, &Else);
  Builder->SetInsertPoint(Then1);
  instrumentAlignedLoad4(Then1, LoadAddr, DefIDs);
  Builder->SetInsertPoint(Then2);
  instrumentUnalignedLoad2(Then2, LoadAddr, DefIDs);
}
void DataFlowIntegritySanitizerPass::instrumentCondAlignedOrUnalignedLoad4(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Instruction *Then1, *Then2, *Else;
  insertIfAddrIsInAlignedRegionElseIfAddrIsInUnalignedRegion(Builder.get(), LoadAddr, Load, &Then1, &Then2, &Else);
  Builder->SetInsertPoint(Then1);
  instrumentAlignedLoad4(Then1, LoadAddr, DefIDs);
  Builder->SetInsertPoint(Then2);
  instrumentUnalignedLoad4(Then2, LoadAddr, DefIDs);
}
void DataFlowIntegritySanitizerPass::instrumentCondAlignedOrUnalignedLoad8(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Instruction *Then1, *Then2, *Else;
  insertIfAddrIsInAlignedRegionElseIfAddrIsInUnalignedRegion(Builder.get(), LoadAddr, Load, &Then1, &Then2, &Else);
  Builder->SetInsertPoint(Then1);
  instrumentAlignedLoad8(Then1, LoadAddr, DefIDs);
  Builder->SetInsertPoint(Then2);
  instrumentUnalignedLoad8(Then2, LoadAddr, DefIDs);
}
void DataFlowIntegritySanitizerPass::instrumentCondAlignedOrUnalignedLoad16(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs) {
  Instruction *Then1, *Then2, *Else;
  insertIfAddrIsInAlignedRegionElseIfAddrIsInUnalignedRegion(Builder.get(), LoadAddr, Load, &Then1, &Then2, &Else);
  Builder->SetInsertPoint(Then1);
  instrumentAlignedLoad16(Then1, LoadAddr, DefIDs);
  Builder->SetInsertPoint(Then2);
  instrumentUnalignedLoad16(Then2, LoadAddr, DefIDs);
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

  SmallVector<Type *, 8> StoreArgTypes{PtrTy, Int16Ty};
  FunctionType *StoreFnTy = FunctionType::get(VoidTy, StoreArgTypes, false);
  SmallVector<Type *, 8> StoreNArgTypes{PtrTy, Int64Ty, Int16Ty};
  FunctionType *StoreNFnTy = FunctionType::get(VoidTy, StoreNArgTypes, false);
  // SmallVector<Type *, 8> LoadArgTypes{PtrTy, Int32Ty};
  SmallVector<Type *, 8> LoadArgTypes{PtrTy, Int16Ty};
  FunctionType *LoadFnTy = FunctionType::get(VoidTy, LoadArgTypes, true);
  // SmallVector<Type *, 8> LoadNArgTypes{PtrTy, Int64Ty, Int32Ty};
  SmallVector<Type *, 8> LoadNArgTypes{PtrTy, Int64Ty, Int16Ty};
  FunctionType *LoadNFnTy = FunctionType::get(VoidTy, LoadNArgTypes, true);
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
        createDfiLoadFn(Ope.Operand, Ope.SizeVal, DefIDs, Kind, DefInst);
        createDfiStoreFn(Info.ID, Ope.Operand, Ope.SizeVal, Kind, DefInst);
      } else {
        LLVM_DEBUG(dbgs() << " - ID(" << Info.ID << "), Operand: " << *Ope.Operand << ", Size: " << Ope.Size << "\n");
        createDfiLoadFn(Ope.Operand, Ope.Size, DefIDs, Kind, DefInst);
        createDfiStoreFn(Info.ID, Ope.Operand, Ope.Size, Kind, DefInst);
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
    // default: llvm_unreachable("Invalid UseDefKind");
    }
  } else {
    switch(Kind) {
    case UseDefKind::Aligned: {
      switch(Size) {
      case 1:    
      case 2:    
      case 4:   instrumentAlignedStore4 (InsertPoint, StoreAddr, DefIDVal); break;
      case 8:   instrumentAlignedStore8 (InsertPoint, StoreAddr, DefIDVal); break;
      case 16:  instrumentAlignedStore16(InsertPoint, StoreAddr, DefIDVal); break;
      default:  Builder->CreateCall(AlignedStoreNFn, NArgs); break;
      }
      NumInstrumentedAlignedStores++;
      break;
    }
    case UseDefKind::Unaligned: {
      switch(Size) {
      case 1:   instrumentUnalignedStore1 (InsertPoint, StoreAddr, DefIDVal); break;
      case 2:   instrumentUnalignedStore2 (InsertPoint, StoreAddr, DefIDVal); break;
      case 4:   instrumentUnalignedStore4 (InsertPoint, StoreAddr, DefIDVal); break;
      case 8:   instrumentUnalignedStore8 (InsertPoint, StoreAddr, DefIDVal); break;
      case 16:  instrumentUnalignedStore16(InsertPoint, StoreAddr, DefIDVal); break;
      default:  Builder->CreateCall(UnalignedStoreNFn, NArgs); break;
      }
      NumInstrumentedUnalignedStores++;
      break;
    } 
    case UseDefKind::AlignedOrUnaligned: {
      switch(Size) {
      case 1:   instrumentAlignedOrUnalignedStore1 (InsertPoint, StoreAddr, DefIDVal); break;
      case 2:   instrumentAlignedOrUnalignedStore2 (InsertPoint, StoreAddr, DefIDVal); break;
      case 4:   instrumentAlignedOrUnalignedStore4 (InsertPoint, StoreAddr, DefIDVal); break;
      case 8:   instrumentAlignedOrUnalignedStore8 (InsertPoint, StoreAddr, DefIDVal); break;
      case 16:  instrumentAlignedOrUnalignedStore16(InsertPoint, StoreAddr, DefIDVal); break;
      default:  Builder->CreateCall(AlignedOrUnalignedStoreNFn, NArgs); break;
      }
      NumInstrumentedAlignedOrUnalignedStores++;
      break;
    }
    case UseDefKind::CondAligned: {
      switch(Size) {
      case 1:   instrumentCondAlignedStore1 (InsertPoint, StoreAddr, DefIDVal); break;
      case 2:   instrumentCondAlignedStore2 (InsertPoint, StoreAddr, DefIDVal); break;
      case 4:   instrumentCondAlignedStore4 (InsertPoint, StoreAddr, DefIDVal); break;
      case 8:   instrumentCondAlignedStore8 (InsertPoint, StoreAddr, DefIDVal); break;
      case 16:  instrumentCondAlignedStore16(InsertPoint, StoreAddr, DefIDVal); break;
      default:  Builder->CreateCall(CondAlignedStoreNFn, NArgs); break;
      }
      NumInstrumentedCondAlignedStores++;
      break;
    }
    case UseDefKind::CondUnaligned: {
      switch(Size) {
      case 1:   instrumentCondUnalignedStore1 (InsertPoint, StoreAddr, DefIDVal); break;
      case 2:   instrumentCondUnalignedStore2 (InsertPoint, StoreAddr, DefIDVal); break;
      case 4:   instrumentCondUnalignedStore4 (InsertPoint, StoreAddr, DefIDVal); break;
      case 8:   instrumentCondUnalignedStore8 (InsertPoint, StoreAddr, DefIDVal); break;
      case 16:  instrumentCondUnalignedStore16(InsertPoint, StoreAddr, DefIDVal); break;
      default:  Builder->CreateCall(CondUnalignedStoreNFn, NArgs); break;
      }
      NumInstrumentedCondUnalignedStores++;
      break;
    }
    case UseDefKind::CondAlignedOrUnaligned: {
      switch(Size) {
      case 1:   instrumentCondAlignedOrUnalignedStore1 (InsertPoint, StoreAddr, DefIDVal); break;
      case 2:   instrumentCondAlignedOrUnalignedStore2 (InsertPoint, StoreAddr, DefIDVal); break;
      case 4:   instrumentCondAlignedOrUnalignedStore4 (InsertPoint, StoreAddr, DefIDVal); break;
      case 8:   instrumentCondAlignedOrUnalignedStore8 (InsertPoint, StoreAddr, DefIDVal); break;
      case 16:  instrumentCondAlignedOrUnalignedStore16(InsertPoint, StoreAddr, DefIDVal); break;
      default:  Builder->CreateCall(CondAlignedOrUnalignedStoreNFn, NArgs); break;
      }
      NumInstrumentedCondAlignedOrUnalignedStores++;
      break;
    }
    // default: llvm_unreachable("Invalid UseDefKind");
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
    // default: llvm_unreachable("Invalid UseDefKind");
    }
  } else {
    switch(Kind) {
    case UseDefKind::Aligned: {
      switch(Size) {
      case 1:
      case 2:
      case 4:   instrumentAlignedLoad4 (InsertPoint, LoadAddr, DefIDs); break;
      case 8:   instrumentAlignedLoad8 (InsertPoint, LoadAddr, DefIDs); break;
      case 16:  instrumentAlignedLoad16(InsertPoint, LoadAddr, DefIDs); break;
      default:  Builder->CreateCall(AlignedLoadNFn, NArgs); break;
      }
      NumInstrumentedAlignedLoads++;
      break;
    }
    case UseDefKind::Unaligned: {
      switch(Size) {
      case 1:   instrumentUnalignedLoad1 (InsertPoint, LoadAddr, DefIDs); break;
      case 2:   instrumentUnalignedLoad2 (InsertPoint, LoadAddr, DefIDs); break;
      case 4:   instrumentUnalignedLoad4 (InsertPoint, LoadAddr, DefIDs); break;
      case 8:   instrumentUnalignedLoad8 (InsertPoint, LoadAddr, DefIDs); break;
      case 16:  instrumentUnalignedLoad16(InsertPoint, LoadAddr, DefIDs); break;
      default:  Builder->CreateCall(UnalignedLoadNFn, NArgs); break;
      }
      NumInstrumentedUnalignedLoads++;
      break;
    } 
    case UseDefKind::AlignedOrUnaligned: {
      switch(Size) {
      case 1:   instrumentAlignedOrUnalignedLoad1 (InsertPoint, LoadAddr, DefIDs); break;
      case 2:   instrumentAlignedOrUnalignedLoad2 (InsertPoint, LoadAddr, DefIDs); break;
      case 4:   instrumentAlignedOrUnalignedLoad4 (InsertPoint, LoadAddr, DefIDs); break;
      case 8:   instrumentAlignedOrUnalignedLoad8 (InsertPoint, LoadAddr, DefIDs); break;
      case 16:  instrumentAlignedOrUnalignedLoad16(InsertPoint, LoadAddr, DefIDs); break;
      default:  Builder->CreateCall(AlignedOrUnalignedLoadNFn, NArgs); break;
      }
      NumInstrumentedAlignedOrUnalignedLoads++;
      break;
    }
    case UseDefKind::CondAligned: {
      switch(Size) {
      case 1:   
      case 2:   
      case 4:   instrumentCondAlignedLoad4 (InsertPoint, LoadAddr, DefIDs); break;
      case 8:   instrumentCondAlignedLoad8 (InsertPoint, LoadAddr, DefIDs); break;
      case 16:  instrumentCondAlignedLoad16(InsertPoint, LoadAddr, DefIDs); break;
      default:  Builder->CreateCall(CondAlignedLoadNFn, NArgs); break;
      }
      NumInstrumentedCondAlignedLoads++;
      break;
    }
    case UseDefKind::CondUnaligned: {
      switch(Size) {
      case 1:   instrumentCondUnalignedLoad1 (InsertPoint, LoadAddr, DefIDs); break;
      case 2:   instrumentCondUnalignedLoad2 (InsertPoint, LoadAddr, DefIDs); break;
      case 4:   instrumentCondUnalignedLoad4 (InsertPoint, LoadAddr, DefIDs); break;
      case 8:   instrumentCondUnalignedLoad8 (InsertPoint, LoadAddr, DefIDs); break;
      case 16:  instrumentCondUnalignedLoad16(InsertPoint, LoadAddr, DefIDs); break;
      default:  Builder->CreateCall(CondUnalignedLoadNFn, NArgs); break;
      }
      NumInstrumentedCondUnalignedLoads++;
      break;
    }
    case UseDefKind::CondAlignedOrUnaligned: {
      switch(Size) {
      case 1:   instrumentCondAlignedOrUnalignedLoad1 (InsertPoint, LoadAddr, DefIDs); break;
      case 2:   instrumentCondAlignedOrUnalignedLoad2 (InsertPoint, LoadAddr, DefIDs); break;
      case 4:   instrumentCondAlignedOrUnalignedLoad4 (InsertPoint, LoadAddr, DefIDs); break;
      case 8:   instrumentCondAlignedOrUnalignedLoad8 (InsertPoint, LoadAddr, DefIDs); break;
      case 16:  instrumentCondAlignedOrUnalignedLoad16(InsertPoint, LoadAddr, DefIDs); break;
      default:  Builder->CreateCall(CondAlignedOrUnalignedLoadNFn, NArgs); break;
      }
      NumInstrumentedCondAlignedOrUnalignedLoads++;
      break;
    }
    // default: llvm_unreachable("Invalid UseDefKind");
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
