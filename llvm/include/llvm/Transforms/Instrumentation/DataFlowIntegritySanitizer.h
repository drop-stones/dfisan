#ifndef LLVM_TRANSFORMS_INSTRUMENTATION_DATAFLOWINTEGRITY_SANITIZER_H
#define LLVM_TRANSFORMS_INSTRUMENTATION_DATAFLOWINTEGRITY_SANITIZER_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/IRBuilder.h"

namespace SVF {
class StoreVFGNode;
class LoadVFGNode;
class UseDefChain;
} // namespace SVF

namespace llvm {

class DataFlowIntegritySanitizerPass : public PassInfoMixin<DataFlowIntegritySanitizerPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);

private:
  FunctionCallee DfiInitFn, DfiStoreFn, DfiLoadFn;
  Type *VoidTy, *ArgTy, *PtrTy;

  /// Initialize member variables.
  void initializeSanitizerFuncs(Module &M);

  /// Create Ctor functions which call DfiInitFn and Insert it to global ctors.
  void insertDfiInitFn(Module &M, IRBuilder<> &Builder);

  /// Insert DfiStoreFn after each store instruction.
  void insertDfiStoreFn(IRBuilder<> &Builder, SVF::UseDefChain *UseDef, const SVF::StoreVFGNode *StoreNode);

  /// Insert DfiLoadFn before each load instruction.
  void insertDfiLoadFn(IRBuilder<> &Builder, const SVF::LoadVFGNode *LoadNode, SmallVector<Value *, 8> &DefIDs);
};

} // namespace llvm

#endif