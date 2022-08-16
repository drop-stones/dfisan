#ifndef LLVM_ANALYSIS_DG_PASSES_DFIMEMORYSSA_H
#define LLVM_ANALYSIS_DG_PASSES_DFIMEMORYSSA_H

#include "dg/MemorySSA/MemorySSA.h"
#include "llvm/ReadWriteGraph/LLVMReadWriteGraphBuilder.h"
#include <llvm/Support/raw_ostream.h>

namespace dg {
namespace dda {

class DfiMemorySSATransformation : public MemorySSATransformation {
  std::vector<RWNode *> findDefinitions(RWBBlock *block, const DefSite &ds) override;
  std::vector<RWNode *> findDefinitions(RWNode *node, const DefSite &ds) override;
  std::vector<RWNode *> findDefinitions(RWNode *node) override;

  void addUncoveredFromPredecessors(RWBBlock *block, Definitions &D,
                                    const DefSite &ds,
                                    std::vector<RWNode *> &defs) override;

  RWNode *createPhi(Definitions &D, const DefSite &ds, RWNodeType type = RWNodeType::PHI) override;
  Definitions &getBBlockDefinitions(RWBBlock *b, const DefSite *ds = nullptr) override;

public:
  DfiMemorySSATransformation(ReadWriteGraph &&Graph, const DataDependenceAnalysisOptions &Opts, LLVMReadWriteGraphBuilder *RWBuilder)
    : MemorySSATransformation(std::move(Graph), Opts), RWBuilder(RWBuilder) { llvm::errs() << "DfiMemorySSATransformation\n"; }
  
  void run() override;
  std::vector<RWNode *> getDefinitions(RWNode *where, RWNode *mem,
                                       const Offset &off, const Offset &len) override;

private:
  const LLVMReadWriteGraphBuilder *RWBuilder;
};

} // namespace dda
} // namespace dg

#endif
