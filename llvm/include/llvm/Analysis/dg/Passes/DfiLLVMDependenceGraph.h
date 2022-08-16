#ifndef LLVM_ANALYSIS_DG_PASSES_DFILLVMDEPENDENCEGRAPH_H
#define LLVM_ANALYSIS_DG_PASSES_DFILLVMDEPENDENCEGRAPH_H

#include "dg/llvm/LLVMDependenceGraph.h"

namespace dg {

class DfiLLVMDependenceGraph : public LLVMDependenceGraph {
public:
  DfiLLVMDependenceGraph(bool threads = false) : LLVMDependenceGraph(threads) {}

  void addDefUseEdges(bool preserveDbg = true) override;
};

} // namespace dg

#endif
