#include <llvm/IR/GlobalVariable.h>

#include "dg/Passes/DfiDataDependenceAnalysis.h"
#include "Passes/DfiReadWriteGraphBuilder.h"
#include "dg/llvm/LLVMDependenceGraph.h"

namespace dg {
namespace dda {

DfiDataDependenceAnalysis::DfiDataDependenceAnalysis(const llvm::Module *M,
                                                     dg::LLVMPointerAnalysis *PTA,
                                                     LLVMDataDependenceAnalysisOptions Opts)
  : LLVMDataDependenceAnalysis(M, PTA, Opts) {
  if (builder != nullptr)
    delete(builder);
  builder = createBuilder();
}

LLVMReadWriteGraphBuilder *DfiDataDependenceAnalysis::createBuilder() {
  assert(m && pta);
  return new DfiReadWriteGraphBuilder(m, pta, _options);
}

} // namespace dda
} // namespace dg