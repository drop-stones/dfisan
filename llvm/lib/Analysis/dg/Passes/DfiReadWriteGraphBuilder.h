#ifndef LLVM_ANALYSIS_DG_PASSES_DFIREADWRITEGRAPHBUILDER_H
#define LLVM_ANALYSIS_DG_PASSES_DFIREADWRITEGRAPHBUILDER_H

#include "dg/Passes/UseDefBuilder.h"
#include "llvm/ReadWriteGraph/LLVMReadWriteGraphBuilder.h"

namespace dg {
namespace dda {

class DfiReadWriteGraphBuilder : public LLVMReadWriteGraphBuilder {
private:
  NodesSeq<RWNode> createNode(const llvm::Value *) override;

protected:
  void buildSubgraph(const llvm::Function &F) override;

public:
  DfiReadWriteGraphBuilder(const llvm::Module *M, dg::LLVMPointerAnalysis *PTA,
                           const LLVMDataDependenceAnalysisOptions &Opts)
    : LLVMReadWriteGraphBuilder(M, PTA, Opts) {}
};

} // namespace dda
} // namespace dg

#endif