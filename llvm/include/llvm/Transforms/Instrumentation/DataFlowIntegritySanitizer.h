#ifndef LLVM_TRANSFORMS_INSTRUMENTATION_DATAFLOWINTEGRITY_SANITIZER_H
#define LLVM_TRANSFORMS_INSTRUMENTATION_DATAFLOWINTEGRITY_SANITIZER_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/IRBuilder.h"

namespace dg {
class UseDefBuilder;
using DefID = uint16_t;
} // namespace dg

namespace llvm {

class DataFlowIntegritySanitizerPass : public PassInfoMixin<DataFlowIntegritySanitizerPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);

private:
  FunctionCallee DfiInitFn, DfiStoreNFn, DfiLoadNFn,
                 DfiStore1Fn, DfiStore2Fn, DfiStore4Fn, DfiStore8Fn, DfiStore16Fn,
                 DfiLoad1Fn, DfiLoad2Fn, DfiLoad4Fn, DfiLoad8Fn, DfiLoad16Fn;
  Type *VoidTy, *ArgTy, *PtrTy, *Int8Ty, *Int32Ty;
  dg::UseDefBuilder *UseDef;
  Module *M;
  std::unique_ptr<IRBuilder<>> Builder;

  /// Initialize member variables.
  void initializeSanitizerFuncs();

  /// Create Ctor functions which call DfiInitFn and Insert it to global ctors.
  void insertDfiInitFn();

  /// Insert DfiStoreFn after each store instruction.
  void insertDfiStoreFn(Value *Def);

  /// Insert DfiLoadFn before each load instruction.
  void insertDfiLoadFn(Value *Use, SmallVector<Value *, 8> &DefIDs);

  /// Create a function call to DfiStoreFn from llvm::Value.
  void createDfiStoreFn(dg::DefID DefID, Value *StoreTarget, unsigned Size);
  void createDfiStoreFn(dg::DefID DefID, Value *StoreTarget, unsigned Size, Instruction *InsertPoint);

  /// Create a function call to DfiLoadFn.
  void createDfiLoadFn(Value *LoadTarget, unsigned Size, SmallVector<Value *, 8> &DefIDs);
  void createDfiLoadFn(Value *LoadTarget, unsigned Size, SmallVector<Value *, 8> &DefIDs, Instruction *InsertPoint);

  /// Create DfiStoreFn for aggregate data.
  //void createDfiStoreFnForAggregateData(Value *Store);
  //void createDfiStoreFnForAggregateData(Value *Store, Instruction *InsertPoint);

  /// Create the StructGEP and return the target register.
  /// Called only by createDfiStoreFnForAggregateData().
  //Value *createStructGep(oreVFGNode *StoreNode, const std::vector<struct SVF::FieldOffset *> &OffsetVec);
};

} // namespace llvm

#endif