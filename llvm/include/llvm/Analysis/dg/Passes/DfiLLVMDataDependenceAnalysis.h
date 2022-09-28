#ifndef LLVM_ANALYSIS_DG_PASSES_DFILLVMDATADEPENDENCEANALYSIS_H
#define LLVM_ANALYSIS_DG_PASSES_DFILLVMDATADEPENDENCEANALYSIS_H

#include "dg/llvm/DataDependence/DataDependence.h"
#include "dg/Passes/DfiProtectInfo.h"

namespace dg {
namespace dda {

class DfiReadWriteGraphBuilder;

class DfiLLVMDataDependenceAnalysis : public LLVMDataDependenceAnalysis {
private:
  DfiProtectInfo *ProtectInfo = nullptr;

  LLVMReadWriteGraphBuilder *createBuilder();
  DataDependenceAnalysis *createDDA() override;

public:
  DfiLLVMDataDependenceAnalysis(const llvm::Module *M, 
                                dg::LLVMPointerAnalysis *PTA,
                                DfiProtectInfo *ProtectInfo,
                                LLVMDataDependenceAnalysisOptions Opts = {});
  
  // We can override isDef conditions.
  bool isDef(const llvm::Value *Val) const override {
    const auto *Node = getNode(Val);
    return Node && Node->isDef();
  }
};

} // namespace dda
} // namespace dg

#endif