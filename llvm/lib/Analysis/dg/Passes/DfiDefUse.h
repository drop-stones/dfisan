#ifndef LLVM_ANALYSIS_DG_PASSES_USE_DEF_ANALYSIS_H
#define LLVM_ANALYSIS_DG_PASSES_USE_DEF_ANALYSIS_H

#include "dg/llvm/LLVMDependenceGraph.h"
#include "dg/llvm/LLVMNode.h"
#include "dg/Passes/DfiProtectInfo.h"
#include "dg/Passes/DfiUtils.h"
#include "llvm/DefUse/DefUse.h"

namespace dg {

struct DfiDefUseKind {
  DfiDefUseKind() : IsDef(false), IsUse(false), IsAligned(false), IsUnaligned(false), IsNoTarget(false) {}

  /// Def kind
  bool isAlignedOnlyDef()         { return IsDef &&  IsAligned && !IsUnaligned && !IsNoTarget; }
  bool isUnalignedOnlyDef()       { return IsDef && !IsAligned &&  IsUnaligned && !IsNoTarget; }
  bool isBothOnlyDef()            { return IsDef &&  IsAligned &&  IsUnaligned && !IsNoTarget; }
  bool isAlignedOrNoTargetDef()   { return IsDef &&  IsAligned && !IsUnaligned &&  IsNoTarget; }
  bool isUnalignedOrNoTargetDef() { return IsDef && !IsAligned &&  IsUnaligned &&  IsNoTarget; }
  bool isBothOrNoTargetDef()      { return IsDef &&  IsAligned &&  IsUnaligned &&  IsNoTarget; }

  /// Use kind
  bool isAlignedOnlyUse()         { return IsUse &&  IsAligned && !IsUnaligned && !IsNoTarget; }
  bool isUnalignedOnlyUse()       { return IsUse && !IsAligned &&  IsUnaligned && !IsNoTarget; }
  bool isBothOnlyUse()            { return IsUse &&  IsAligned &&  IsUnaligned && !IsNoTarget; }
  bool isAlignedOrNoTargetUse()   { return IsUse &&  IsAligned && !IsUnaligned &&  IsNoTarget; }
  bool isUnalignedOrNoTargetUse() { return IsUse && !IsAligned &&  IsUnaligned &&  IsNoTarget; }
  bool isBothOrNoTargetUse()      { return IsUse &&  IsAligned &&  IsUnaligned &&  IsNoTarget; }

  // Unused funcs
  bool isNoKind()   { return !IsAligned && !IsUnaligned && !IsNoTarget; }
  bool isNoTarget() { return !IsAligned && !IsUnaligned && IsNoTarget; }

  bool IsDef;
  bool IsUse;
  bool IsAligned;
  bool IsUnaligned;
  bool IsNoTarget;
};

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
  DfiDefUseKind analyzeDefKind(llvm::Value *Val);
  DfiDefUseKind analyzeUseKind(llvm::Value *Val);

  // Helper function to analyze UseDefKind
  void analyzeDefUseKindFromDefSiteSet(DfiDefUseKind &Kind, dda::DefSiteSet &DefSites);
};

} // namespace dg

#endif
