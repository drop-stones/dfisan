#ifndef LLVM_ANALYSIS_DG_PASSES_USE_DEF_ANALYSIS_H
#define LLVM_ANALYSIS_DG_PASSES_USE_DEF_ANALYSIS_H

#include "dg/llvm/LLVMDependenceGraph.h"
#include "dg/llvm/LLVMNode.h"
#include "dg/Passes/DfiProtectInfo.h"
#include "dg/Passes/DfiUtils.h"
#include "llvm/DefUse/DefUse.h"

namespace dg {

class DfiDefUseAnalysis : public LLVMDefUseAnalysis {
  DfiProtectInfo *ProtectInfo;
public:
  DfiDefUseAnalysis(LLVMDependenceGraph *dg, LLVMDataDependenceAnalysis *rd,
                    LLVMPointerAnalysis *pta, DfiProtectInfo *ProtectInfo)
    : LLVMDefUseAnalysis(dg, rd, pta), ProtectInfo(ProtectInfo) {}
  
  bool runOnNode(LLVMNode *Node, LLVMNode *Prev) override;
  
protected:
  void addDataDependencies(LLVMNode *Node) override;

private:
  bool isAlignedDef(llvm::Value *Def);
  bool isAlignedUse(llvm::Value *Use);
  bool isUnalignedDef(llvm::Value *Def);
  bool isUnalignedUse(llvm::Value *Use);
};

} // namespace dg

#endif
