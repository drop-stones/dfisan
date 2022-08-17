#ifndef LLVM_ANALYSIS_DG_PASSES_DFILLVMDEPENDENCEGRAPH_H
#define LLVM_ANALYSIS_DG_PASSES_DFILLVMDEPENDENCEGRAPH_H

#include "dg/llvm/LLVMDependenceGraph.h"
#include "dg/Passes/DfiProtectInfo.h"

namespace dg {

class DfiLLVMDependenceGraph : public LLVMDependenceGraph {
  DfiProtectInfo &ProtectInfo;
public:
  DfiLLVMDependenceGraph(DfiProtectInfo &ProtectInfo, bool threads = false)
    : LLVMDependenceGraph(threads), ProtectInfo(ProtectInfo) {}

  void addDefUseEdges(bool preserveDbg = true) override;
};

} // namespace dg

#endif
