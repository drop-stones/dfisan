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
    bool IsAligned = isAlignedUse(Val);
    bool IsUnaligned = isUnalignedUse(Val);
    if (IsAligned && IsUnaligned) {
      ProtectInfo->insertBothUse(Inst);
    } else if (IsAligned) {
      ProtectInfo->insertAlignedUse(Inst);
    } else if (IsUnaligned) {
      ProtectInfo->insertUnalignedUse(Inst);
    } else {
      return false; // no use of targets
    }
    addDataDependencies(Node);
  } else if (RD->isDef(Val)) {
    bool IsAligned = isAlignedDef(Val);
    bool IsUnaligned = isUnalignedDef(Val);
    if (IsAligned && IsUnaligned) {
      ProtectInfo->insertBothDef(Val);
    } else if (IsAligned) {
      ProtectInfo->insertAlignedDef(Val);
    } else if (IsUnaligned) {
      ProtectInfo->insertUnalignedDef(Val);
    }
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

} // namespace dg