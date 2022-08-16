#ifndef LLVM_ANALYSIS_DG_PASSES_DFIDATADEPENDENCEANALYSIS_H
#define LLVM_ANALYSIS_DG_PASSES_DFIDATADEPENDENCEANALYSIS_H

#include "dg/DataDependence/DataDependence.h"
#include "dg/Passes/DfiMemorySSA.h"
#include "llvm/ReadWriteGraph/LLVMReadWriteGraphBuilder.h"

namespace dg {
namespace dda {

class DfiDataDependenceAnalysis : public DataDependenceAnalysis {
public:
  DfiDataDependenceAnalysis(ReadWriteGraph &&Graph, const DataDependenceAnalysisOptions &Opts, LLVMReadWriteGraphBuilder *RWBuilder)
    : DataDependenceAnalysis(new DfiMemorySSATransformation(std::move(Graph), Opts, RWBuilder), Opts) {}
};

} // namespace dda
} // namespace dg

#endif
