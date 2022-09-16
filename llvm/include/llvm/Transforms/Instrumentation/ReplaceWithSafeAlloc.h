#ifndef LLVM_TRANSFORMS_INSTRUMENTATION_REPLACE_WITH_SAFE_ALLOC_H
#define LLVM_TRANSFORMS_INSTRUMENTATION_REPLACE_WITH_SAFE_ALLOC_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class ReplaceWithSafeAllocPass : public PassInfoMixin<ReplaceWithSafeAllocPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
};

} // namespace llvm

#endif
