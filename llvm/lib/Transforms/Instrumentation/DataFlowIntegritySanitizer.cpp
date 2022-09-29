#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/TypeFinder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Transforms/Instrumentation/DataFlowIntegritySanitizer.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"  /* appendToUsed */
#include "llvm/Support/Debug.h"

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

PreservedAnalyses
DataFlowIntegritySanitizerPass::run(Module &M, ModuleAnalysisManager &MAM) {
  LLVM_DEBUG(dbgs() << "DataFlowIntegritySanitizerPass: Insert check functions to enforce data flow integrity\n");

  this->M = &M;
  Builder = std::make_unique<IRBuilder<>>(M.getContext());

  initializeSanitizerFuncs();
  insertDfiInitFn();

  auto &Result = MAM.getResult<UseDefAnalysisPass>(M);
  ProtectInfo = Result.getProtectInfo();

  // Instrument unsafe access
  for (auto &Func : M.getFunctionList()) {
    for (auto &Inst : instructions(&Func)) {
      if (isUnsafeAccess(&Inst))
        insertCheckUnsafeAccessFn(&Inst);
    }
  }

  if (Result.emptyResult()) // skip use-def instrumentation
    return PreservedAnalyses::all();

  DG = Result.getDG();
  DDA = Result.getDDA();
  Opts = &DDA->getOptions();

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

  SmallVector<Type *, 8> StoreArgTypes{PtrTy, Int16Ty};
  FunctionType *StoreFnTy = FunctionType::get(VoidTy, StoreArgTypes, false);
  SmallVector<Type *, 8> StoreNArgTypes{PtrTy, Int64Ty, Int16Ty};
  FunctionType *StoreNFnTy = FunctionType::get(VoidTy, StoreNArgTypes, false);
  SmallVector<Type *, 8> LoadArgTypes{PtrTy, Int32Ty};
  FunctionType *LoadFnTy = FunctionType::get(VoidTy, LoadArgTypes, true);
  SmallVector<Type *, 8> LoadNArgTypes{PtrTy, Int64Ty, Int32Ty};
  FunctionType *LoadNFnTy = FunctionType::get(VoidTy, LoadNArgTypes, true);
  FunctionType *CheckUnsafeAcessFnTy = FunctionType::get(VoidTy, {Int64Ty}, false);

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

  // Test for dlmalloc
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
    } else if (MemCpyInst *Memcpy = dyn_cast<MemCpyInst>(DefInst)) {
      auto *StoreTarget = Memcpy->getOperand(0);
      auto *SizeVal = Memcpy->getOperand(2);
      createDfiStoreFn(ProtectInfo->getDefID(Memcpy), StoreTarget, SizeVal, Kind, Memcpy->getNextNode());
    } else if (MemSetInst *Memset = dyn_cast<MemSetInst>(DefInst)) {
      auto *StoreTarget = Memset->getOperand(0);
      auto *SizeVal = Memset->getOperand(2);
      createDfiStoreFn(ProtectInfo->getDefID(Memset), StoreTarget, SizeVal, Kind, Memset->getNextNode());
    } else if (CallInst *Call = dyn_cast<CallInst>(DefInst)) {
      auto *Callee = Call->getCalledFunction();
      if (Opts->getAllocationFunction(Callee->getName().str()) == dg::AllocationFunction::CALLOC) {
        auto *Nmem = Call->getOperand(0);
        auto *Size = Call->getOperand(1);
        Builder->SetInsertPoint(Call->getNextNode());
        auto *SizeVal = Builder->CreateNUWMul(Nmem, Size);
        createDfiStoreFn(ProtectInfo->getDefID(Call), Def, SizeVal, Kind);
      } else if (Callee->getName() == "fgets") {
        auto *StoreTarget = Call->getOperand(0);
        auto *SizeVal = Call->getOperand(1);
        createDfiStoreFn(ProtectInfo->getDefID(Call), StoreTarget, SizeVal, Kind, Call->getNextNode());
      } else if (Callee->getName() == "__isoc99_sscanf") {
        for (unsigned Idx = 2; Idx < Call->arg_size(); Idx++) {
          auto *StoreTarget = Call->getOperand(Idx);
          unsigned Size = M->getDataLayout().getTypeStoreSize(StoreTarget->getType()->getNonOpaquePointerElementType());
          createDfiStoreFn(ProtectInfo->getDefID(Call), StoreTarget, Size, Kind, Call->getNextNode());
        }
      } else if (Callee->getName() == "read") {
        auto *StoreTarget = Call->getOperand(1);
        auto *SizeVal = Def;
        llvm::errs() << "Instrument " << Callee->getName() << "\n";
        llvm::errs() << " - Target: " << *StoreTarget << ", Size: " << *SizeVal << "\n";
        createDfiStoreFn(ProtectInfo->getDefID(Call), StoreTarget, SizeVal, Kind, Call->getNextNode());
      } else {
        llvm::errs() << "No support Def function: " << Callee->getName() << "\n";
        // llvm_unreachable("No support Def function");
      }
    } else if (AllocaInst *Alloca = dyn_cast<AllocaInst>(Def)) {
      // Do nothing
    } else {
      llvm::errs() << "No support DefInst: " << *DefInst << "\n";
      // llvm_unreachable("No support DefInst");
    }
  } else if (GlobalVariable *GlobVar = dyn_cast<GlobalVariable>(Def)) {
    unsigned Size = M->getDataLayout().getTypeStoreSize(GlobVar->getType()->getNonOpaquePointerElementType());
    createDfiStoreFn(ProtectInfo->getDefID(GlobVar), GlobVar, Size, Kind, Ctor->getEntryBlock().getTerminator());
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
  ValueVector DefIDs;
  for (auto *Def : DDA->getLLVMDefinitions(Use)) {
    if (ProtectInfo->hasDefID(Def)) { // Def may be a no-target instruction.
      Value *DefID = ConstantInt::get(Int32Ty, ProtectInfo->getDefID(Def), false);
      DefIDs.push_back(DefID);
    }
  }

  if (LoadInst *Load = dyn_cast<LoadInst>(Use)) {
    auto *LoadTarget = Load->getPointerOperand();
    unsigned Size = M->getDataLayout().getTypeStoreSize(LoadTarget->getType()->getNonOpaquePointerElementType());
    createDfiLoadFn(LoadTarget, Size, DefIDs, Kind, Load);
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
    break;
  }
  // default: llvm_unreachable("Invalid UseDefKind");
  }
}

inline bool DataFlowIntegritySanitizerPass::isUnsafeAccess(Instruction *Inst) {
  Value *Target = nullptr;
  if (auto *Load = dyn_cast<LoadInst>(Inst)) {
    if (ProtectInfo->hasUse(Load))
      return false;
    Target = Load->getPointerOperand();
  } else if (auto *Store = dyn_cast<StoreInst>(Inst)) {
    if (ProtectInfo->hasDef(Store))
      return false;
    Target = Store->getPointerOperand();
  } else {  // TODO: function calls which use or def
    return false;
  }

  if (isa<GlobalValue>(Target)) // global value are constant pointers
    return false;
  // if (isa<AllocaInst>(Target))  // access to local variables are safe
  //   return false;
  
  return true;
}

void DataFlowIntegritySanitizerPass::insertCheckUnsafeAccessFn(Instruction *Inst) {
  Value *Target = nullptr;
  if (auto *Load = dyn_cast<LoadInst>(Inst)) {
    Target = Load->getPointerOperand();
  } else if (auto *Store = dyn_cast<StoreInst>(Inst)) {
    Target = Store->getPointerOperand();
  } else {  // TODO: function calls which use or def
    llvm_unreachable("No unsafe access");
  }

  Builder->SetInsertPoint(Inst);
  Value *TgtAddr = Builder->CreatePtrToInt(Target, PtrTy);
  Builder->CreateCall(CheckUnsafeAccessFn, {TgtAddr});
}
