#ifndef LLVM_TRANSFORMS_INSTRUMENTATION_DATAFLOWINTEGRITY_SANITIZER_H
#define LLVM_TRANSFORMS_INSTRUMENTATION_DATAFLOWINTEGRITY_SANITIZER_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/IRBuilder.h"

namespace dg {
// class UseDefBuilder;
class LLVMDependenceGraph;
class DfiProtectInfo;
using DefID = uint16_t;
namespace dda {
class LLVMDataDependenceAnalysis;
} // namespace dda
} // namespace dg

namespace llvm {

using ValueVector = SmallVector<Value *, 8>;

class DataFlowIntegritySanitizerPass : public PassInfoMixin<DataFlowIntegritySanitizerPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);

private:
  FunctionCallee DfiInitFn, DfiStoreNFn, DfiLoadNFn,
                 DfiStore1Fn, DfiStore2Fn, DfiStore4Fn, DfiStore8Fn, DfiStore16Fn,
                 DfiLoad1Fn, DfiLoad2Fn, DfiLoad4Fn, DfiLoad8Fn, DfiLoad16Fn,
                 AlignedStoreNFn, AlignedStore1Fn, AlignedStore2Fn, AlignedStore4Fn, AlignedStore8Fn, AlignedStore16Fn,
                 UnalignedStoreNFn, UnalignedStore1Fn, UnalignedStore2Fn, UnalignedStore4Fn, UnalignedStore8Fn, UnalignedStore16Fn,
                 AlignedOrUnalignedStoreNFn, AlignedOrUnalignedStore1Fn, AlignedOrUnalignedStore2Fn,
                 AlignedOrUnalignedStore4Fn, AlignedOrUnalignedStore8Fn, AlignedOrUnalignedStore16Fn,
                 CondAlignedStoreNFn, CondAlignedStore1Fn, CondAlignedStore2Fn, CondAlignedStore4Fn, CondAlignedStore8Fn, CondAlignedStore16Fn,
                 CondUnalignedStoreNFn, CondUnalignedStore1Fn, CondUnalignedStore2Fn, CondUnalignedStore4Fn, CondUnalignedStore8Fn, CondUnalignedStore16Fn,
                 CondAlignedOrUnalignedStoreNFn, CondAlignedOrUnalignedStore1Fn, CondAlignedOrUnalignedStore2Fn,
                 CondAlignedOrUnalignedStore4Fn, CondAlignedOrUnalignedStore8Fn, CondAlignedOrUnalignedStore16Fn,
                 AlignedLoadNFn, AlignedLoad1Fn, AlignedLoad2Fn, AlignedLoad4Fn, AlignedLoad8Fn, AlignedLoad16Fn,
                 UnalignedLoadNFn, UnalignedLoad1Fn, UnalignedLoad2Fn, UnalignedLoad4Fn, UnalignedLoad8Fn, UnalignedLoad16Fn,
                 AlignedOrUnalignedLoadNFn, AlignedOrUnalignedLoad1Fn, AlignedOrUnalignedLoad2Fn,
                 AlignedOrUnalignedLoad4Fn, AlignedOrUnalignedLoad8Fn, AlignedOrUnalignedLoad16Fn,
                 CondAlignedLoadNFn, CondAlignedLoad1Fn, CondAlignedLoad2Fn, CondAlignedLoad4Fn, CondAlignedLoad8Fn, CondAlignedLoad16Fn,
                 CondUnalignedLoadNFn, CondUnalignedLoad1Fn, CondUnalignedLoad2Fn, CondUnalignedLoad4Fn, CondUnalignedLoad8Fn, CondUnalignedLoad16Fn,
                 CondAlignedOrUnalignedLoadNFn, CondAlignedOrUnalignedLoad1Fn, CondAlignedOrUnalignedLoad2Fn,
                 CondAlignedOrUnalignedLoad4Fn, CondAlignedOrUnalignedLoad8Fn, CondAlignedOrUnalignedLoad16Fn;
  Type *VoidTy, *ArgTy, *PtrTy, *Int8Ty, *Int16Ty, *Int32Ty, *Int64Ty;
  // dg::UseDefBuilder *UseDef;
  dg::LLVMDependenceGraph *DG = nullptr;
  dg::dda::LLVMDataDependenceAnalysis *DDA = nullptr;
  dg::DfiProtectInfo *ProtectInfo = nullptr;
  Module *M = nullptr;
  std::unique_ptr<IRBuilder<>> Builder{nullptr};
  Function *Ctor = nullptr;

  enum class UseDefKind {
    Aligned, Unaligned, AlignedOrUnaligned,
    CondAligned, CondUnaligned, CondAlignedOrUnaligned,
  };

  /// Initialize member variables.
  void initializeSanitizerFuncs();

  /// Create Ctor functions which call DfiInitFn and Insert it to global ctors.
  void insertDfiInitFn();

  /// Insert DfiStoreFn after each store instruction.
  void insertDfiStoreFn(Value *Def, UseDefKind Kind);

  /// Insert DfiLoadFn before each load instruction.
  void insertDfiLoadFn(Instruction *Use, UseDefKind Kind);

  /// Create a function call to DfiStoreFn from llvm::Value.
  void createDfiStoreFn(dg::DefID DefID, Value *StoreTarget, unsigned Size, UseDefKind Kind, Instruction *InsertPoint = nullptr);
  void createDfiStoreFn(dg::DefID DefID, Value *StoreTarget, Value *SizeVal, UseDefKind Kind, Instruction *InsertPoint = nullptr);

  /// Create a function call to DfiLoadFn.
  void createDfiLoadFn(Value *LoadTarget, unsigned Size, ValueVector &DefIDs, UseDefKind Kind, Instruction *InsertPoint = nullptr);
  void createDfiLoadFn(Value *LoadTarget, Value *SizeVal, ValueVector &DefIDs, UseDefKind Kind, Instruction *InsertPoint = nullptr);

  /// Create DfiStoreFn for aggregate data.
  //void createDfiStoreFnForAggregateData(Value *Store);
  //void createDfiStoreFnForAggregateData(Value *Store, Instruction *InsertPoint);

  /// Create the StructGEP and return the target register.
  /// Called only by createDfiStoreFnForAggregateData().
  //Value *createStructGep(oreVFGNode *StoreNode, const std::vector<struct SVF::FieldOffset *> &OffsetVec);
};

} // namespace llvm

#endif