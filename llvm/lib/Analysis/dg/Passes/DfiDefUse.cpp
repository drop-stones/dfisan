#include "DfiDefUse.h"

#include "llvm/Support/Debug.h"
#define DEBUG_TYPE "dfi-def-use"

namespace dg {

/// Copy from DefUse.cpp
/// Add def-use edges between instruction and its operands
static void handleOperands(const Instruction *Inst, LLVMNode *node) {
    LLVMDependenceGraph *dg = node->getDG();
    assert(Inst == node->getKey());

    for (auto I = Inst->op_begin(), E = Inst->op_end(); I != E; ++I) {
        auto *op = dg->getNode(*I);
        if (!op)
            continue;
        const auto &subs = op->getSubgraphs();
        if (!subs.empty() && !op->isVoidTy()) {
            for (auto *s : subs) {
                s->getExit()->addDataDependence(node);
            }
        }
        // 'node' uses 'op', so we want to add edge 'op'-->'node',
        // that is, 'op' is used in 'node' ('node' is a user of 'op')
        op->addUseDependence(node);
    }
}

// Collect uses of targets into ProtectInfo.
bool DfiDefUseAnalysis::runOnNode(LLVMNode *Node, LLVMNode *Prev) {
  Value *Val = Node->getKey();
  LLVM_DEBUG(llvm::dbgs() << "DfiDefUseAnalysis::" << __func__ << ": " << *Val << "\n");

  // just add direct def-use edges to every instruction
  if (auto *Inst = dyn_cast<Instruction>(Val))
    handleOperands(Inst, Node);

  // collect def-use of targets
  if (RD->isUse(Val)) {
    assert(isa<Instruction>(Val) && "Use is not instruction");
    Instruction *Inst = dyn_cast<Instruction>(Val);
    DfiDefUseKind Kind = analyzeUseKind(Inst);
    if (Kind.isAlignedOnlyUse())
      ProtectInfo->insertAlignedOnlyUse(Inst);
    else if (Kind.isUnalignedOnlyUse())
      ProtectInfo->insertUnalignedOnlyUse(Inst);
    else if (Kind.isBothOnlyUse())
      ProtectInfo->insertBothOnlyUse(Inst);
    else if (Kind.isAlignedOrNoTargetUse())
      ProtectInfo->insertAlignedOrNoTargetUse(Inst);
    else if (Kind.isUnalignedOrNoTargetUse())
      ProtectInfo->insertUnalignedOrNoTargetUse(Inst);
    else if (Kind.isBothOrNoTargetUse())
      ProtectInfo->insertBothOrNoTargetUse(Inst);
    else
      return false; // no use of targets
    addDataDependencies(Node);
  }

  return false;
}

// Collect defs of targets into ProtectInfo.
void DfiDefUseAnalysis::addDataDependencies(LLVMNode *Node) {
  LLVMDefUseAnalysis::addDataDependencies(Node);

  auto *Val = Node->getValue();
  auto Defs = RD->getLLVMDefinitions(Val);
  LLVM_DEBUG(dbgs() << "DfiDefUseAnalysis::" << __func__ << ": " << *Val << "\n");
  for (auto *Def : Defs) {
    LLVM_DEBUG(dbgs() << " - Def: " << *Def << "\n");
    assert(RD->isDef(Def) && "No def value");
    DfiDefUseKind Kind = analyzeDefKind(Def);
    if (Kind.isAlignedOnlyDef())
      ProtectInfo->insertAlignedOnlyDef(Def);
    else if (Kind.isUnalignedOnlyDef())
      ProtectInfo->insertUnalignedOnlyDef(Def);
    else if (Kind.isBothOnlyDef())
      ProtectInfo->insertBothOnlyDef(Def);
    else if (Kind.isAlignedOrNoTargetDef())
      ProtectInfo->insertAlignedOrNoTargetDef(Def);
    else if (Kind.isUnalignedOrNoTargetDef())
      ProtectInfo->insertUnalignedOrNoTargetDef(Def);
    else if (Kind.isBothOrNoTargetDef())
      ProtectInfo->insertBothOrNoTargetDef(Def);
  }
}

DfiDefUseKind DfiDefUseAnalysis::analyzeDefKind(Value *Val) {
  LLVM_DEBUG(dbgs() << "DfiDefUseAnalysis::" << __func__ << ": " << *Val << "\n");
  assert(RD->isDef(Val) && "No def value");
  struct DfiDefUseKind Kind;
  auto *RWNode = RD->getNode(Val);
  Kind.IsDef = true;
  for (auto &TargetSite : RWNode->getDefines()) {
    auto *Target = (llvm::Value *)RD->getValue(TargetSite.target);
    Kind.IsAligned   |= ProtectInfo->hasAlignedTarget(Target);
    Kind.IsUnaligned |= ProtectInfo->hasUnalignedTarget(Target);
    Kind.IsNoTarget  |= !(ProtectInfo->hasTarget(Target));
    LLVM_DEBUG(dbgs() << " - Target: " << (Kind.IsAligned ? "aligned" : "") << " "
                                       << (Kind.IsUnaligned ? "unaligned" : "") << " "
                                       << (Kind.IsNoTarget ? "no-target" : "") << " "
                                       << *Target << "\n");
  }
  return Kind;
}

DfiDefUseKind DfiDefUseAnalysis::analyzeUseKind(Value *Val) {
  LLVM_DEBUG(dbgs() << "DfiDefUseAnalysis::" << __func__ << ": " << *Val << "\n");
  assert(RD->isUse(Val) && "No use value");
  struct DfiDefUseKind Kind;
  auto *RWNode = RD->getNode(Val);
  Kind.IsUse = true;
  for (auto &TargetSite : RWNode->getUses()) {
    auto *Target = (llvm::Value *)RD->getValue(TargetSite.target);
    Kind.IsAligned   |= ProtectInfo->hasAlignedTarget(Target);
    Kind.IsUnaligned |= ProtectInfo->hasUnalignedTarget(Target);
    Kind.IsNoTarget  |= !(ProtectInfo->hasTarget(Target));
    LLVM_DEBUG(dbgs() << " - Target: " << (Kind.IsAligned ? "aligned" : "") << " "
                                       << (Kind.IsUnaligned ? "unaligned" : "") << " "
                                       << (Kind.IsNoTarget ? "no-target" : "") << " "
                                       << *Target << "\n");
  }
  return Kind;
}
} // namespace dg