#ifndef LLVM_ANALYSIS_DG_PASSES_DFILLVMDEPENDENCEGRAPHBUILDER_H
#define LLVM_ANALYSIS_DG_PASSES_DFILLVMDEPENDENCEGRAPHBUILDER_H

#include "dg/llvm/LLVMDependenceGraphBuilder.h"
#include "dg/Passes/DfiProtectInfo.h"
#include "dg/Passes/DfiLLVMDataDependenceAnalysis.h"
#include "dg/Passes/DfiLLVMDependenceGraph.h"

namespace dg {
namespace llvmdg {

class DfiLLVMDependenceGraphBuilder : public LLVMDependenceGraphBuilder {
  DfiProtectInfo *ProtectInfo;
public:
  DfiLLVMDependenceGraphBuilder(DfiProtectInfo *ProtectInfo, llvm::Module *M, const LLVMDependenceGraphOptions &Opts = {})
    : LLVMDependenceGraphBuilder(M, Opts), ProtectInfo(ProtectInfo) {
    _DDA.reset(new dda::DfiLLVMDataDependenceAnalysis(M, _PTA.get(), ProtectInfo, _options.DDAOptions));
    _dg.reset(new DfiLLVMDependenceGraph(ProtectInfo, Opts.threads));
    llvm::errs() << "BuildType: " << ((Opts.PTAOptions.isSVF()) ? "SVF" : "DG") << "\n";
  }
};

} // namespace llvmdg
} // namespace dg

#endif