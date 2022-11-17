#ifndef LLVM_TRANSFORMS_INSTRUMENTATION_DATAFLOWINTEGRITY_SANITIZER_H
#define LLVM_TRANSFORMS_INSTRUMENTATION_DATAFLOWINTEGRITY_SANITIZER_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/MDBuilder.h"

namespace SVF {
class ProtectInfo;
using DefID = uint16_t;
} // namespace SVF

namespace llvm {

using ValueVector = SmallVector<Value *, 8>;

class DataFlowIntegritySanitizerPass : public PassInfoMixin<DataFlowIntegritySanitizerPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);

private:
  FunctionCallee DfiInitFn, DfiFiniFn, DfiStoreNFn, DfiLoadNFn,
                 DfiStore1Fn, DfiStore2Fn, DfiStore4Fn, DfiStore8Fn, DfiStore16Fn,
                 DfiLoad1Fn, DfiLoad2Fn, DfiLoad4Fn, DfiLoad8Fn, DfiLoad16Fn,
                 // Store
                 AlignedStoreNFn, AlignedStore1Fn, AlignedStore2Fn, AlignedStore4Fn, AlignedStore8Fn, AlignedStore16Fn,
                 UnalignedStoreNFn, UnalignedStore1Fn, UnalignedStore2Fn, UnalignedStore4Fn, UnalignedStore8Fn, UnalignedStore16Fn,
                 AlignedOrUnalignedStoreNFn, AlignedOrUnalignedStore1Fn, AlignedOrUnalignedStore2Fn,
                 AlignedOrUnalignedStore4Fn, AlignedOrUnalignedStore8Fn, AlignedOrUnalignedStore16Fn,
                 CondAlignedStoreNFn, CondAlignedStore1Fn, CondAlignedStore2Fn, CondAlignedStore4Fn, CondAlignedStore8Fn, CondAlignedStore16Fn,
                 CondUnalignedStoreNFn, CondUnalignedStore1Fn, CondUnalignedStore2Fn, CondUnalignedStore4Fn, CondUnalignedStore8Fn, CondUnalignedStore16Fn,
                 CondAlignedOrUnalignedStoreNFn, CondAlignedOrUnalignedStore1Fn, CondAlignedOrUnalignedStore2Fn,
                 CondAlignedOrUnalignedStore4Fn, CondAlignedOrUnalignedStore8Fn, CondAlignedOrUnalignedStore16Fn,
                 // Load
                 AlignedLoadNFn, AlignedLoad1Fn, AlignedLoad2Fn, AlignedLoad4Fn, AlignedLoad8Fn, AlignedLoad16Fn,
                 UnalignedLoadNFn, UnalignedLoad1Fn, UnalignedLoad2Fn, UnalignedLoad4Fn, UnalignedLoad8Fn, UnalignedLoad16Fn,
                 AlignedOrUnalignedLoadNFn, AlignedOrUnalignedLoad1Fn, AlignedOrUnalignedLoad2Fn,
                 AlignedOrUnalignedLoad4Fn, AlignedOrUnalignedLoad8Fn, AlignedOrUnalignedLoad16Fn,
                 CondAlignedLoadNFn, CondAlignedLoad1Fn, CondAlignedLoad2Fn, CondAlignedLoad4Fn, CondAlignedLoad8Fn, CondAlignedLoad16Fn,
                 CondUnalignedLoadNFn, CondUnalignedLoad1Fn, CondUnalignedLoad2Fn, CondUnalignedLoad4Fn, CondUnalignedLoad8Fn, CondUnalignedLoad16Fn,
                 CondAlignedOrUnalignedLoadNFn, CondAlignedOrUnalignedLoad1Fn, CondAlignedOrUnalignedLoad2Fn,
                 CondAlignedOrUnalignedLoad4Fn, CondAlignedOrUnalignedLoad8Fn, CondAlignedOrUnalignedLoad16Fn,
                 // Check and Set
                 AlignedRaceStoreNFn, AlignedRaceStore1Fn, AlignedRaceStore2Fn, AlignedRaceStore4Fn, AlignedRaceStore8Fn, AlignedRaceStore16Fn,
                 UnalignedRaceStoreNFn, UnalignedRaceStore1Fn, UnalignedRaceStore2Fn, UnalignedRaceStore4Fn, UnalignedRaceStore8Fn, UnalignedRaceStore16Fn,
                 AlignedOrUnalignedRaceStoreNFn, AlignedOrUnalignedRaceStore1Fn, AlignedOrUnalignedRaceStore2Fn,
                 AlignedOrUnalignedRaceStore4Fn, AlignedOrUnalignedRaceStore8Fn, AlignedOrUnalignedRaceStore16Fn,
                 CondAlignedRaceStoreNFn, CondAlignedRaceStore1Fn, CondAlignedRaceStore2Fn, CondAlignedRaceStore4Fn, CondAlignedRaceStore8Fn, CondAlignedRaceStore16Fn,
                 CondUnalignedRaceStoreNFn, CondUnalignedRaceStore1Fn, CondUnalignedRaceStore2Fn, CondUnalignedRaceStore4Fn, CondUnalignedRaceStore8Fn, CondUnalignedRaceStore16Fn,
                 CondAlignedOrUnalignedRaceStoreNFn, CondAlignedOrUnalignedRaceStore1Fn, CondAlignedOrUnalignedRaceStore2Fn,
                 CondAlignedOrUnalignedRaceStore4Fn, CondAlignedOrUnalignedRaceStore8Fn, CondAlignedOrUnalignedRaceStore16Fn,
                 // Others
                 CheckUnsafeAccessFn, InvalidSafeAccessReportFn, InvalidUseReportFn;
  Type *VoidTy, *PtrTy, *Int8Ty, *Int16Ty, *Int32Ty, *Int64Ty, *Int8PtrTy;
  SVF::ProtectInfo *ProtInfo = nullptr;
  Module *M = nullptr;
  std::unique_ptr<IRBuilder<>> Builder{nullptr};
  std::unique_ptr<MDBuilder> MdBuilder{nullptr};
  Function *Ctor = nullptr, *Dtor = nullptr;

  enum class UseDefKind {
    Aligned, Unaligned, AlignedOrUnaligned,
    CondAligned, CondUnaligned, CondAlignedOrUnaligned,
  };

  ///
  //  Runtime check functions
  ///
  Instruction *generateCrashCode(Instruction *InsertBefore, Value *Addr, ValueVector &DefIDs, bool IsUnsafe = false);
  void insertIfThenAndErrorReport(Value *Cond, Instruction *InsertPoint, Value *LoadAddr, ValueVector &DefIDs);

  /// Instrument an unsafe access
  void instrumentUnsafeAccess(Instruction *OrigInst, Value *Addr);

  /// Instrument safe store
  void instrumentAlignedStore (Instruction *InsertPoint, Value *StoreAddr, Value *DefID, unsigned Size);
  void instrumentAlignedStoreN(Instruction *InsertPoint, Value *StoreAddr, Value *DefID, Value *SizeVal = nullptr, unsigned Size = 0);
  /// Instrument safe unaligned store
  void instrumentUnalignedStore (Instruction *InsertPoint, Value *StoreAddr, Value *DefID, unsigned Size);
  void instrumentUnalignedStoreN(Instruction *InsertPoint, Value *StoreAddr, Value *DefID);
  /// Instrument safe aligned or unaligned store
  void instrumentAlignedOrUnalignedStore (Instruction *InsertPoint, Value *StoreAddr, Value *DefID, unsigned Size);
  void instrumentAlignedOrUnalignedStoreN(Instruction *InsertPoint, Value *StoreAddr, Value *DefID);
  /// Instrument conditional aligned store
  void instrumentCondAlignedStore (Instruction *InsertPoint, Value *StoreAddr, Value *DefID, unsigned Size);
  void instrumentCondAlignedStoreN(Instruction *InsertPoint, Value *StoreAddr, Value *DefID);
  /// Instrument conditional unaligned store
  void instrumentCondUnalignedStore (Instruction *InsertPoint, Value *StoreAddr, Value *DefID, unsigned Size);
  void instrumentCondUnalignedStoreN(Instruction *InsertPoint, Value *StoreAddr, Value *DefID);
  /// Instrument conditional aligned or unaligned store
  void instrumentCondAlignedOrUnalignedStore (Instruction *InsertPoint, Value *StoreAddr, Value *DefID, unsigned Size);
  void instrumentCondAlignedOrUnalignedStoreN(Instruction *InsertPoint, Value *StoreAddr, Value *DefID);

  /// Instrument safe aligned load
  void instrumentAlignedLoad (Instruction *Load, Value *LoadAddr, ValueVector &DefIDs, unsigned Size);
  void instrumentAlignedLoadN(Instruction *Load, Value *LoadAddr, unsigned Size, ValueVector &DefIDs);
  /// Instrument safe unaligned load
  void instrumentUnalignedLoad (Instruction *Load, Value *LoadAddr, ValueVector &DefIDs, unsigned Size);
  void instrumentUnalignedLoadN(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs);
  /// Instrument safe aligned or unaligned load
  void instrumentAlignedOrUnalignedLoad (Instruction *Load, Value *LoadAddr, ValueVector &DefIDs, unsigned Size);
  void instrumentAlignedOrUnalignedLoadN(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs);
  /// Instrument conditional aligned load
  void instrumentCondAlignedLoad (Instruction *Load, Value *LoadAddr, ValueVector &DefIDs, unsigned Size);
  void instrumentCondAlignedLoadN(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs);
  /// Instrument conditional unaligned load
  void instrumentCondUnalignedLoad (Instruction *Load, Value *LoadAddr, ValueVector &DefIDs, unsigned Size);
  void instrumentCondUnalignedLoadN(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs);
  /// Instrument conditional aligned or unaligned load
  void instrumentCondAlignedOrUnalignedLoad (Instruction *Load, Value *LoadAddr, ValueVector &DefIDs, unsigned Size);
  void instrumentCondAlignedOrUnalignedLoadN(Instruction *Load, Value *LoadAddr, ValueVector &DefIDs);

  /// Instrument write-write race aligned store
  void instrumentAlignedRaceStore(Instruction *InsertPoint, Value *Addr, Value *DefID, ValueVector &DefIDs, unsigned Size);
  /// Instrument write-write race unaligned store
  void instrumentUnalignedRaceStore(Instruction *InsertPoint, Value *Addr, Value *DefID, ValueVector &DefIDs, unsigned Size);
  /// Instrument write-write race aligned or unaligned store
  void instrumentAlignedOrUnalignedRaceStore(Instruction *InsertPoint, Value *Addr, Value *DefID, ValueVector &DefIDs, unsigned Size);
  /// Instrument write-write race conditional aligned store
  void instrumentCondAlignedRaceStore(Instruction *InsertPoint, Value *Addr, Value *DefID, ValueVector &DefIDs, unsigned Size);
  /// Instrument write-write race conditional unaligned store
  void instrumentCondUnalignedRaceStore(Instruction *InsertPoint, Value *Addr, Value *DefID, ValueVector &DefIDs, unsigned Size);
  /// Instrument write-write race conditional aligned or unaligned store
  void instrumentCondAlignedOrUnalignedRaceStore(Instruction *InsertPoint, Value *Addr, Value *DefID, ValueVector &DefIDs, unsigned Size);

  /// Instrument write-read race aligned load
  void instrumentAlignedRaceLoad(Instruction *InsertPoint, Value *LoadAddr, ValueVector &DefIDs, unsigned Size);
  /// Instrument write-read race unaligned load
  void instrumentUnalignedRaceLoad(Instruction *InsertPoint, Value *LoadAddr, ValueVector &DefIDs, unsigned Size);
  /// Instrument write-read race aligned or unaligned load
  void instrumentAlignedOrUnalignedRaceLoad(Instruction *InsertPoint, Value *LoadAddr, ValueVector &DefIDs, unsigned Size);
  /// Instrument write-read race conditional aligned load
  void instrumentCondAlignedRaceLoad(Instruction *InsertPoint, Value *LoadAddr, ValueVector &DefIDs, unsigned Size);
  /// Instrument write-read race conditional unaligned load
  void instrumentCondUnalignedRaceLoad(Instruction *InsertPoint, Value *LoadAddr, ValueVector &DefIDs, unsigned Size);
  /// Instrument write-read race conditional aligned or unaligned load
  void instrumentCondAlignedOrUnalignedRaceLoad(Instruction *InsertPoint, Value *LoadAddr, ValueVector &DefIDs, unsigned Size);

  ///
  //  Instrumentation functions
  ///

  /// Initialize member variables.
  void initializeSanitizerFuncs();

  /// Create Ctor functions which call DfiInitFn and Insert it to global ctors.
  void insertDfiInitFn();

  /// Create Dtor functions which call DfiFiniFn
  void insertDfiFiniFn();

  /// Insert DfiStoreFn after each store instruction.
  void insertDfiStoreFn(Value *Def, UseDefKind Kind);

  /// Insert Check and Set functions before each may-write-write-race store instruction.
  void instrumentMayWriteWriteRaceStore(Value *Def, UseDefKind Kind);

  /// Insert GetShadow before may-write-read load instructions
  /// and check after them.
  void instrumentMayWriteReadRaceLoad(Value *Use, UseDefKind Kind, ValueVector &DefIDs);

  /// Insert DfiLoadFn before each load instruction.
  void insertDfiLoadFn(Value *Use, UseDefKind Kind);

  /// Create a function call to DfiStoreFn from llvm::Value.
  void createDfiStoreFn(SVF::DefID DefID, Value *StoreTarget, unsigned Size, UseDefKind Kind, Instruction *InsertPoint = nullptr);
  void createDfiStoreFn(SVF::DefID DefID, Value *StoreTarget, Value *SizeVal, UseDefKind Kind, Instruction *InsertPoint = nullptr);

  /// Create a function call to DfiLoadFn.
  void createDfiLoadFn(Value *LoadTarget, unsigned Size, ValueVector &DefIDs, UseDefKind Kind, Instruction *InsertPoint = nullptr);
  void createDfiLoadFn(Value *LoadTarget, Value *SizeVal, ValueVector &DefIDs, UseDefKind Kind, Instruction *InsertPoint = nullptr);

  /// Create a function call to check write-write race.
  void createDfiRaceStoreFn(SVF::DefID DefID, Value *Target, unsigned Size, ValueVector &DefIDs, UseDefKind Kind, Instruction *InsertPoint = nullptr);
  void createDfiRaceStoreFn(SVF::DefID DefID, Value *Target, Value *SizeVal, ValueVector &DefIDs, UseDefKind Kind, Instruction *InsertPoint = nullptr);

  /// Create a function call to check write-read race.
  void createDfiRaceLoadFn(Value *LoadTarget, unsigned Size, ValueVector &DefIDs, UseDefKind Kind, Instruction *InsertPoint = nullptr);
  void createDfiRaceLoadFn(Value *LoadTarget, Value *SizeVal, ValueVector &DefIDs, UseDefKind Kind, Instruction *InsertPoint = nullptr);

  /// Return true if the target is unsafe
  inline bool isUnsafeAccessTarget(Value *Target);
  /// Add a target if the targets is unsafe
  inline void addIfUnsafeAccessTarget(ValueVector &Targets, Value *Target);
  /// Get unsafe access target or return nullptr
  inline void getUnsafeAccessTargets(Instruction *Inst, ValueVector &Targets);
  /// Insert unsafe access func before each unsafe access.
  void insertCheckUnsafeAccessFn(Instruction *Inst, ValueVector &Targets);

  /// Insert global init (because global targets are not initialized)
  void insertGlobalInit(GlobalVariable *GlobVar);
};

} // namespace llvm

#endif