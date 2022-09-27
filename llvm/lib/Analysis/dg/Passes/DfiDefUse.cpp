#include "DfiDefUse.h"

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

bool DfiDefUseAnalysis::runOnNode(LLVMNode *Node, LLVMNode *Prev) {
  Value *Val = Node->getKey();

  // just add direct def-use edges to every instruction
  if (auto *Inst = dyn_cast<Instruction>(Val))
    handleOperands(Inst, Node);

  // collect def-use of targets
  if (RD->isUse(Val)) {
    assert(isa<Instruction>(Val) && "Use is not instruction");
    Instruction *Inst = dyn_cast<Instruction>(Val);
    DfiDefUseKind Kind = analyzeDefUseKind(Inst);
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
  } else if (RD->isDef(Val)) {
    DfiDefUseKind Kind = analyzeDefUseKind(Val);
    if (Kind.isAlignedOnlyDef())
      ProtectInfo->insertAlignedOnlyDef(Val);
    else if (Kind.isUnalignedOnlyDef())
      ProtectInfo->insertUnalignedOnlyDef(Val);
    else if (Kind.isBothOnlyDef())
      ProtectInfo->insertBothOnlyDef(Val);
    else if (Kind.isAlignedOrNoTargetDef())
      ProtectInfo->insertAlignedOrNoTargetDef(Val);
    else if (Kind.isUnalignedOrNoTargetDef())
      ProtectInfo->insertUnalignedOrNoTargetDef(Val);
    else if (Kind.isBothOrNoTargetDef())
      ProtectInfo->insertBothOrNoTargetDef(Val);
  }

  return false;

/*
  if (!RD->isUse(Val))
    return LLVMDefUseAnalysis::runOnNode(Node, Prev);
  assert(isa<Instruction>(Val) && "runOnNode: not instruction use");
  Instruction *Inst = dyn_cast<Instruction>(Val);

  // Selective DataFlow analysis
  auto *RWNode = RD->getNode(Val);
  for (auto &Use : RWNode->getUses()) {
    auto *UseVal = RD->getValue(Use.target);
    if (ProtectInfo->hasTarget((llvm::Value *)UseVal)) {
      // llvm::errs() << "runOnNode: " << *Val << "\n";
      // llvm::errs() << " - Use: " << *UseVal << "\n";
      ProtectInfo->insertUse(Inst);
      auto Ret = LLVMDefUseAnalysis::runOnNode(Node, Prev);
      for (auto *Def : RD->getLLVMDefinitions(Val)) {
        bool IsAligned = isAlignedDef(Def);
        bool IsUnaligned = isUnalignedDef(Def);
        if (IsAligned && IsUnaligned) {
          ProtectInfo->insertBothDef(Def);
        } else if (IsAligned) {
          ProtectInfo->insertAlignedDef(Def);
        } else if (IsUnaligned) {
          ProtectInfo->insertUnalignedDef(Def);
        }
      }
      return Ret;
    }
  }
  return false;
*/
}

void DfiDefUseAnalysis::addDataDependencies(LLVMNode *Node) {
  LLVMDefUseAnalysis::addDataDependencies(Node);
}

/*
bool DfiDefUseAnalysis::isAlignedDef(Value *Def) {
  assert(RD->isDef(Def) && "No def instruction");
  bool Ret = false;
  auto *DefNode = RD->getNode(Def);
  for (auto &DefSite : DefNode->getDefines()) {
    auto *Target = RD->getValue(DefSite.target);
    Ret |= ProtectInfo->hasAlignedTarget((llvm::Value *)Target);
  }
  return Ret;
}

bool DfiDefUseAnalysis::isUnalignedDef(Value *Def) {
  assert(RD->isDef(Def) && "No def instruction");
  bool Ret = false;
  auto *DefNode = RD->getNode(Def);
  for (auto &DefSite : DefNode->getDefines()) {
    auto *Target = RD->getValue(DefSite.target);
    Ret |= ProtectInfo->hasUnalignedTarget((llvm::Value *)Target);
  }
  return Ret;
}

bool DfiDefUseAnalysis::isNoTargetDef(Value *Def) {
  assert(RD->isDef(Def) && "No def instruction");
  bool Ret = false;
  auto *DefNode = RD->getNode(Def);
  for (auto &DefSite : DefNode->getDefines()) {
    auto *Target = RD->getValue(DefSite.target);
    Ret |= !(ProtectInfo->hasTarget((llvm::Value *)Target));
  }
  return Ret;
}

bool DfiDefUseAnalysis::isAlignedUse(Value *Use) {
  assert(RD->isUse(Use) && "No use instruction");
  bool Ret = false;
  auto *UseNode = RD->getNode(Use);
  for (auto &UseSite : UseNode->getUses()) {
    auto *Target = RD->getValue(UseSite.target);
    Ret |= ProtectInfo->hasAlignedTarget((llvm::Value *)Target);
  }
  return Ret;
}

bool DfiDefUseAnalysis::isUnalignedUse(Value *Use) {
  assert(RD->isUse(Use) && "No use instruction");
  bool Ret = false;
  auto *UseNode = RD->getNode(Use);
  for (auto &UseSite : UseNode->getUses()) {
    auto *Target = RD->getValue(UseSite.target);
    Ret |= ProtectInfo->hasUnalignedTarget((llvm::Value *)Target);
  }
  return Ret;
}

bool DfiDefUseAnalysis::isNoTargetUse(Value *Use) {
  assert(RD->isUse(Use) && "No use instruction");
  bool Ret = false;
  auto *UseNode = RD->getNode(Use);
  for (auto &UseSite : UseNode->getUses()) {
    auto *Target = RD->getValue(UseSite.target);
    Ret |= !(ProtectInfo->hasTarget((llvm::Value *)Target));
  }
  return Ret;
}
*/

DfiDefUseKind
DfiDefUseAnalysis::analyzeDefUseKind(Value *Val) {
  struct DfiDefUseKind Kind;
  auto *RWNode = RD->getNode(Val);
  dg::dda::DefSiteSet *Targets;
  if (RD->isDef(Val)) {
    Kind.IsDef = true;
    Targets = &RWNode->getDefines();
  }
  if (RD->isUse(Val)) {
    Kind.IsUse = true;
    Targets = &RWNode->getUses();
  }
  for (auto &TargetSite : *Targets) {
    auto *Target = (llvm::Value *)RD->getValue(TargetSite.target);
    Kind.IsAligned   |= ProtectInfo->hasAlignedTarget(Target);
    Kind.IsUnaligned |= ProtectInfo->hasUnalignedTarget(Target);
    Kind.IsNoTarget  |= !(ProtectInfo->hasTarget(Target));
  }
  return Kind;
}


} // namespace dg