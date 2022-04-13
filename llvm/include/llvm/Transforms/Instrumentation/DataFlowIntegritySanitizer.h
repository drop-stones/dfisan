#ifndef LLVM_TRANSFORMS_INSTRUMENTATION_DATAFLOWINTEGRITY_SANITIZER_H
#define LLVM_TRANSFORMS_INSTRUMENTATION_DATAFLOWINTEGRITY_SANITIZER_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/IRBuilder.h"

namespace SVF {
class StoreVFGNode;
class LoadVFGNode;
} // namespace SVF

namespace llvm {

class DataFlowIntegritySanitizerPass : public PassInfoMixin<DataFlowIntegritySanitizerPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);

private:
  FunctionCallee DfiStoreFn, DfiLoadFn;
  Type *VoidTy, *ArgTy, *PtrTy;

  void initializeSanitizerFuncs(Module &M);
  void insertDfiStoreFn(IRBuilder<> &Builder, const SVF::StoreVFGNode *StoreNode);
  void insertDfiLoadFn(IRBuilder<> &Builder, const SVF::LoadVFGNode *LoadNode, SmallVector<Value *, 8> &DefIDs);
};

} // namespace llvm

#endif