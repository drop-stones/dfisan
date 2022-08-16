#ifndef LLVM_ANALYSIS_DG_PASSES_DFILLVMDEPENDENCEGRAPHBUILDER_H
#define LLVM_ANALYSIS_DG_PASSES_DFILLVMDEPENDENCEGRAPHBUILDER_H

#include "dg/llvm/LLVMDependenceGraphBuilder.h"
#include "dg/Passes/DfiLLVMDataDependenceAnalysis.h"
#include "dg/Passes/DfiLLVMDependenceGraph.h"

namespace dg {
namespace llvmdg {

class DfiLLVMDependenceGraphBuilder : public LLVMDependenceGraphBuilder {
public:
  DfiLLVMDependenceGraphBuilder(llvm::Module *M, const LLVMDependenceGraphOptions &Opts = {})
    : LLVMDependenceGraphBuilder(M, Opts) {
    _DDA.reset(new dda::DfiLLVMDataDependenceAnalysis(M, _PTA.get(), _options.DDAOptions));
    _dg.reset(new DfiLLVMDependenceGraph(Opts.threads));
    llvm::errs() << "BuildType: " << ((Opts.PTAOptions.isSVF()) ? "SVF" : "DG") << "\n";
  }
};

} // namespace llvmdg
} // namespace dg

#endif