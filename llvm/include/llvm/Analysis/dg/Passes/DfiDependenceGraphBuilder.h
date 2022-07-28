#ifndef LLVM_ANALYSIS_DG_PASSES_DFIDEPENDENCEGRAPHBUILDER_H
#define LLVM_ANALYSIS_DG_PASSES_DFIDEPENDENCEGRAPHBUILDER_H

#include "dg/llvm/LLVMDependenceGraphBuilder.h"
#include "dg/Passes/DfiDataDependenceAnalysis.h"

namespace dg {
namespace llvmdg {

class DfiDependenceGraphBuilder : public LLVMDependenceGraphBuilder {
public:
  DfiDependenceGraphBuilder(llvm::Module *M, const LLVMDependenceGraphOptions &Opts = {})
    : LLVMDependenceGraphBuilder(M, Opts) {
    _DDA.reset(new dda::DfiDataDependenceAnalysis(M, _PTA.get(), _options.DDAOptions));
  }
};

} // namespace llvmdg
} // namespace dg

#endif