#include "DfiDefUse.h"

#include "llvm/Support/Debug.h"
#define DEBUG_TYPE "dfi-def-use"

namespace dg {

/// Return true if the value access memory using byval pointer
static bool accessByVal(const Value *Val) {
  Value *Ptr = getAccessObj(Val);
  if (Ptr == nullptr) return false;

  if (auto *Arg = dyn_cast<Argument>(Ptr)) {
    if (Arg->hasByValAttr()) {
      LLVM_DEBUG(dbgs() << "Found byval access: " << *Val << "\n");
      return true;
    }
  }
  return false;
}

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
    if (accessByVal(Val)) return false;   // byval is not protection target
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
  /// Copy from LLVMDefUseAnalysis::addDataDependencies().
  static std::set<const llvm::Value *> reported_mappings;

  auto *Val = Node->getValue();
  auto Defs = RD->getLLVMDefinitions(Val);

  // add data dependence
  for (auto *Def : Defs) {
      LLVMNode *rdNode = dg->getNode(Def);
      if (!rdNode) {
          // that means that the Value is not from this graph.
          // We need to add interprocedural edge
          llvm::Function *F = llvm::cast<llvm::Instruction>(Def)
                                      ->getParent()
                                      ->getParent();
          assert(F != nullptr && "Func is nullptr");
          LLVMNode *entryNode = dg->getGlobalNode(F);
          if (entryNode == nullptr) {
              llvm::errs() << "Skip no-called function: " << F->getName() << "\n";
              continue;
          }
          assert(entryNode && "Don't have built function");

          // get the graph where the Node lives
          LLVMDependenceGraph *graph = entryNode->getDG();
          assert(graph != dg && "Cannot find a Node");
          rdNode = graph->getNode(Def);
          if (!rdNode) {
              // llvmutils::printerr("[DU] error: DG doesn't have val: ", Def);
              llvm::errs() << "[DU] error: DG doesn't have Val: " << *Def << "\n";
              abort();
              return;
          }
      }

      assert(rdNode);
      rdNode->addDataDependence(Node);
  }

  /// Collect defs of targets into ProtectInfo.
  if (accessByVal(Val)) return;   // byval is not protection target
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
  analyzeDefUseKindFromDefSiteSet(Kind, RWNode->getDefines());
  analyzeDefUseKindFromDefSiteSet(Kind, RWNode->getOverwrites());
  return Kind;
}

DfiDefUseKind DfiDefUseAnalysis::analyzeUseKind(Value *Val) {
  LLVM_DEBUG(dbgs() << "DfiDefUseAnalysis::" << __func__ << ": " << *Val << "\n");
  assert(RD->isUse(Val) && "No use value");
  struct DfiDefUseKind Kind;
  auto *RWNode = RD->getNode(Val);
  Kind.IsUse = true;
  analyzeDefUseKindFromDefSiteSet(Kind, RWNode->getUses());
  return Kind;
}

void DfiDefUseAnalysis::analyzeDefUseKindFromDefSiteSet(DfiDefUseKind &Kind, dg::dda::DefSiteSet &DefSites) {
  if (DefSites.empty())
    return;

  for (auto &TargetSite : DefSites) {
    auto *Target = (llvm::Value *)RD->getValue(TargetSite.target);
    LLVM_DEBUG(dbgs() << " - Target: " << *Target << "\n");
    Kind.IsAligned   |= ProtectInfo->hasAlignedTarget(Target);
    Kind.IsUnaligned |= ProtectInfo->hasUnalignedTarget(Target);
    Kind.IsNoTarget  |= !(ProtectInfo->hasTarget(Target));
  }
  LLVM_DEBUG(dbgs() << " - Kind: " << (Kind.IsAligned ? "aligned" : "") << " "
                                   << (Kind.IsUnaligned ? "unaligned" : "") << " "
                                   << (Kind.IsNoTarget ? "no-target" : "") << "\n");
}

} // namespace dg