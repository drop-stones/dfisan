#include <llvm/IR/GlobalVariable.h>

#include "dg/Passes/DfiLLVMDataDependenceAnalysis.h"
#include "dg/Passes/DfiDataDependenceAnalysis.h"
#include "Passes/DfiReadWriteGraphBuilder.h"
#include "dg/llvm/LLVMDependenceGraph.h"

namespace dg {
namespace dda {

DfiLLVMDataDependenceAnalysis::DfiLLVMDataDependenceAnalysis(const llvm::Module *M,
                                                             dg::LLVMPointerAnalysis *PTA,
                                                             DfiProtectInfo *ProtectInfo,
                                                             LLVMDataDependenceAnalysisOptions Opts)
  : LLVMDataDependenceAnalysis(M, PTA, Opts), ProtectInfo(ProtectInfo) {
  if (builder != nullptr)
    delete(builder);
  builder = createBuilder();
}

LLVMReadWriteGraphBuilder *DfiLLVMDataDependenceAnalysis::createBuilder() {
  assert(m && pta);
  return new DfiReadWriteGraphBuilder(m, pta, _options, ProtectInfo);
}

DataDependenceAnalysis *DfiLLVMDataDependenceAnalysis::createDDA() {
  assert(builder);

  auto Graph = builder->build();
  return new DfiDataDependenceAnalysis(std::move(Graph), _options, builder);
}

} // namespace dda
} // namespace dg