#ifndef LLVM_ANALYSIS_DG_PASSES_DFILLVMDEPENDENCEGRAPHBUILDER_H
#define LLVM_ANALYSIS_DG_PASSES_DFILLVMDEPENDENCEGRAPHBUILDER_H

#include "dg/llvm/LLVMDependenceGraphBuilder.h"
#include "dg/Passes/DfiProtectInfo.h"
#include "dg/Passes/DfiLLVMDataDependenceAnalysis.h"
#include "dg/Passes/DfiLLVMDependenceGraph.h"

namespace dg {
namespace llvmdg {

class DfiLLVMDependenceGraphBuilder : public LLVMDependenceGraphBuilder {
  DfiProtectInfo &ProtectInfo;
public:
  DfiLLVMDependenceGraphBuilder(DfiProtectInfo &ProtectInfo, llvm::Module *M, const LLVMDependenceGraphOptions &Opts = {})
    : LLVMDependenceGraphBuilder(M, Opts), ProtectInfo(ProtectInfo) {
    _DDA.reset(new dda::DfiLLVMDataDependenceAnalysis(M, _PTA.get(), _options.DDAOptions));
    _dg.reset(new DfiLLVMDependenceGraph(ProtectInfo, Opts.threads));
    llvm::errs() << "BuildType: " << ((Opts.PTAOptions.isSVF()) ? "SVF" : "DG") << "\n";
  }

  void runAnalysis() {
    _runPointerAnalysis();
    _runDataDependenceAnalysis();
  }

  std::unique_ptr<LLVMDependenceGraph> &&buildDG() {
    assert(getPTA() != nullptr);
    assert(getDDA() != nullptr);

    // build the graph itself (the nodes, but without edges)
    _dg->build(_M, _PTA.get(), _DDA.get(), _entryFunction);

    // insert the data dependencies edges
    _dg->addDefUseEdges(_options.preserveDbg);

    // compute and fill-in control dependencies
    _runControlDependenceAnalysis();

    if (_options.threads) {
        if (_options.PTAOptions.isSVF()) {
            assert(0 && "Threading needs the DG pointer analysis, SVF is "
                        "not supported yet");
            abort();
        }
        _controlFlowGraph->buildFunction(_entryFunction);
        _runInterferenceDependenceAnalysis();
        _runForkJoinAnalysis();
        _runCriticalSectionAnalysis();
    }

    // verify if the graph is built correctly
    if (_options.verifyGraph && !_dg->verify()) {
        _dg.reset();
        return std::move(_dg);
    }

    return std::move(_dg);
  }
};

} // namespace llvmdg
} // namespace dg

#endif