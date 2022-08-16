#ifndef LLVM_ANALYSIS_DG_PASSES_USE_DEF_ANALYSIS_H
#define LLVM_ANALYSIS_DG_PASSES_USE_DEF_ANALYSIS_H

#include "dg/llvm/LLVMDependenceGraph.h"
#include "dg/llvm/LLVMNode.h"
#include "dg/Passes/DfiUtils.h"
#include "llvm/DefUse/DefUse.h"

namespace dg {

class DfiDefUseAnalysis : public LLVMDefUseAnalysis {
public:
  DfiDefUseAnalysis(LLVMDependenceGraph *dg, LLVMDataDependenceAnalysis *rd,
                    LLVMPointerAnalysis *pta)
    : LLVMDefUseAnalysis(dg, rd, pta) {}
  
  void addDataDependencies(LLVMNode *Node) override {
    LLVMDefUseAnalysis::addDataDependencies(Node);
    printUseDefFromDebugLoc(RD, Node->getValue(), 20, 669); // ok
    printUseDefFromDebugLoc(RD, Node->getValue(), 20, 670); // ok
    printUseDefFromDebugLoc(RD, Node->getValue(), 20, 671); // ng
  }
};

} // namespace dg

#endif
