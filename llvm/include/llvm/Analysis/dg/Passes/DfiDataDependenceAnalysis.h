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
};

} // namespace dda
} // namespace dg

#endif