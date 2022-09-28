#ifndef LLVM_ANALYSIS_DG_PASSES_DFIREADWRITEGRAPHBUILDER_H
#define LLVM_ANALYSIS_DG_PASSES_DFIREADWRITEGRAPHBUILDER_H

#include "dg/Passes/UseDefBuilder.h"
#include "llvm/ReadWriteGraph/LLVMReadWriteGraphBuilder.h"

namespace dg {
namespace dda {

class DfiReadWriteGraphBuilder : public LLVMReadWriteGraphBuilder {
private:
  DfiProtectInfo *ProtectInfo = nullptr;

  NodesSeq<RWNode> createNode(const llvm::Value *) override;
  RWNode *createLoad(const llvm::Instruction *Inst);

protected:
  void buildSubgraph(const llvm::Function &F) override;

public:
  DfiReadWriteGraphBuilder(const llvm::Module *M, dg::LLVMPointerAnalysis *PTA,
                           const LLVMDataDependenceAnalysisOptions &Opts,
                           DfiProtectInfo *ProtectInfo)
    : LLVMReadWriteGraphBuilder(M, PTA, Opts), ProtectInfo(ProtectInfo) {}
};

} // namespace dda
} // namespace dg

#endif