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
                 DfiLoad1Fn, DfiLoad2Fn, DfiLoad4Fn, DfiLoad8Fn, DfiLoad16Fn;
  Type *VoidTy, *ArgTy, *PtrTy, *Int8Ty, *Int32Ty, *Int64Ty;
  // dg::UseDefBuilder *UseDef;
  dg::LLVMDependenceGraph *DG = nullptr;
  dg::dda::LLVMDataDependenceAnalysis *DDA = nullptr;
  dg::DfiProtectInfo *ProtectInfo = nullptr;
  Module *M = nullptr;
  std::unique_ptr<IRBuilder<>> Builder{nullptr};
  Function *Ctor = nullptr;

  /// Initialize member variables.
  void initializeSanitizerFuncs();

  /// Create Ctor functions which call DfiInitFn and Insert it to global ctors.
  void insertDfiInitFn();

  /// Insert DfiStoreFn after each store instruction.
  void insertDfiStoreFn(Value *Def);

  /// Insert DfiLoadFn before each load instruction.
  void insertDfiLoadFn(Value *Use, ValueVector &DefIDs);

  /// Create a function call to DfiStoreFn from llvm::Value.
  void createDfiStoreFn(dg::DefID DefID, Value *StoreTarget, unsigned Size, Instruction *InsertPoint = nullptr);
  void createDfiStoreFn(dg::DefID DefID, Value *StoreTarget, Value *SizeVal, Instruction *InsertPoint = nullptr);

  /// Create a function call to DfiLoadFn.
  void createDfiLoadFn(Value *LoadTarget, unsigned Size, ValueVector &DefIDs);
  void createDfiLoadFn(Value *LoadTarget, unsigned Size, ValueVector &DefIDs, Instruction *InsertPoint);

  /// Create DfiStoreFn for aggregate data.
  //void createDfiStoreFnForAggregateData(Value *Store);
  //void createDfiStoreFnForAggregateData(Value *Store, Instruction *InsertPoint);

  /// Create the StructGEP and return the target register.
  /// Called only by createDfiStoreFnForAggregateData().
  //Value *createStructGep(oreVFGNode *StoreNode, const std::vector<struct SVF::FieldOffset *> &OffsetVec);
};

} // namespace llvm

#endif