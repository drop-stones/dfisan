#ifndef LLVM_ANALYSIS_SVF_SPARSE_VALUE_FLOW_GRAPH_H
#define LLVM_ANALYSIS_SVF_SPARSE_VALUE_FLOW_GRAPH_H

#include "llvm/IR/PassManager.h"

namespace SVF {
  class SVFIR;
  class Andersen;
  class SVFG;
} // namespace SVF

namespace llvm {

struct SparseValueFlowGraphResult {
  SVF::SVFIR *Pag;
  SVF::Andersen *Pta;
  SVF::SVFG *Svfg;

  SparseValueFlowGraphResult(SVF::SVFIR *Pag, SVF::Andersen *Pta, SVF::SVFG *Svfg)
    : Pag(Pag), Pta(Pta), Svfg(Svfg) {}
};

class SparseValueFlowGraph : public AnalysisInfoMixin<SparseValueFlowGraph> {
  friend AnalysisInfoMixin<SparseValueFlowGraph>;

  static AnalysisKey Key;

public:
  using Result = SparseValueFlowGraphResult;
  Result run(Module &M, ModuleAnalysisManager &MAM);
};

class SparseValueFlowGraphDumpPass : public PassInfoMixin<SparseValueFlowGraphDumpPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
};

} // namespace llvm

#endif // LLVM_ANALYSIS_SVF_SPARSE_VALUE_FLOW_GRAPH_H