#ifndef LLVM_ANALYSIS_DG_PASSES_DFIDATADEPENDENCEANALYSIS_H
#define LLVM_ANALYSIS_DG_PASSES_DFIDATADEPENDENCEANALYSIS_H

#include "dg/llvm/DataDependence/DataDependence.h"

namespace dg {
namespace dda {

class DfiReadWriteGraphBuilder;

class DfiDataDependenceAnalysis : public LLVMDataDependenceAnalysis {
private:
  LLVMReadWriteGraphBuilder *createBuilder();

public:
  DfiDataDependenceAnalysis(const llvm::Module *M, 
                            dg::LLVMPointerAnalysis *PTA,
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