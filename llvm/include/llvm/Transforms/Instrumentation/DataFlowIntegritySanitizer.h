#ifndef LLVM_TRANSFORMS_INSTRUMENTATION_DATAFLOWINTEGRITY_SANITIZER_H
#define LLVM_TRANSFORMS_INSTRUMENTATION_DATAFLOWINTEGRITY_SANITIZER_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/IRBuilder.h"

namespace SVF {
class StoreVFGNode;
class LoadVFGNode;
class UseDefChain;
class SVFG;
} // namespace SVF

namespace llvm {

class DataFlowIntegritySanitizerPass : public PassInfoMixin<DataFlowIntegritySanitizerPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);

private:
  FunctionCallee DfiInitFn, DfiStoreNFn, DfiLoadNFn,
                 DfiStore1Fn, DfiStore2Fn, DfiStore4Fn, DfiStore8Fn, DfiStore16Fn,
                 DfiLoad1Fn, DfiLoad2Fn, DfiLoad4Fn, DfiLoad8Fn, DfiLoad16Fn;
  Type *VoidTy, *ArgTy, *PtrTy;
  SVF::SVFG *Svfg;
  SVF::UseDefChain *UseDef;

  /// Initialize member variables.
  void initializeSanitizerFuncs(Module &M);

  /// Create Ctor functions which call DfiInitFn and Insert it to global ctors.
  void insertDfiInitFn(Module &M, IRBuilder<> &Builder);

  /// Insert DfiStoreFn after each store instruction.
  void insertDfiStoreFn(Module &M, IRBuilder<> &Builder, const SVF::StoreVFGNode *StoreNode);

  /// Insert DfiLoadFn before each load instruction.
  void insertDfiLoadFn(Module &M, IRBuilder<> &Builder, const SVF::LoadVFGNode *LoadNode, SmallVector<Value *, 8> &DefIDs);

  /// Create a function call to DfiStoreFn.
  void createDfiStoreFn(Module &M, IRBuilder<> &Builder, const SVF::StoreVFGNode *StoreNode, Value *StorePointer);
  void createDfiStoreFn(Module &M, IRBuilder<> &Builder, const SVF::StoreVFGNode *StoreNode, Value *StorePointer, Instruction *InsertPoint);

  /// Create a function call to DfiLoadFn.
  void createDfiLoadFn(Module &M, IRBuilder<> &Builder, const SVF::LoadVFGNode *LoadNode, Value *LoadPointer, SmallVector<Value *, 8> &DefIDs);
  void createDfiLoadFn(Module &M, IRBuilder<> &Builder, const SVF::LoadVFGNode *LoadNode, Value *LoadPointer, Instruction *InsertPoint, SmallVector<Value *, 8> &DefIDs);

  /// Create the StructGEP and return the target register.
  Value *createStructGep(IRBuilder<> &Builder, const SVF::StoreVFGNode *StoreNode);
  Value *createStructGep(IRBuilder<> &Builder, const SVF::StoreVFGNode *StoreNode, Instruction *InsertPoint);
};

} // namespace llvm

#endif