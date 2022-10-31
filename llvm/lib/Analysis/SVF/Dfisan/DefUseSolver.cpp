#include "MTA/MHP.h"
#include "MTA/LockAnalysis.h"
#include "Dfisan/DefUseSolver.h"

#include "llvm/Support/Debug.h"
#define DEBUG_TYPE "def-use-solver"

using namespace SVF;

/// Access target analysis
const PointsTo &DefUseSolver::collectStoreTarget(NodeID ID) {
  const SVFGNode *Node = Svfg->getSVFGNode(ID);
  assert(SVFUtil::isa<StoreSVFGNode>(Node));
  const StoreSVFGNode *StoreNode = SVFUtil::cast<StoreSVFGNode>(Node);
  const PAGNode *DstNode = StoreNode->getPAGDstNode();
  auto &PtsSet = Pta->getPts(DstNode->getId());
  return PtsSet;
}

const PointsTo &DefUseSolver::collectLoadTarget(NodeID ID) {
  const SVFGNode *Node = Svfg->getSVFGNode(ID);
  assert(SVFUtil::isa<LoadSVFGNode>(Node));
  const LoadSVFGNode *LoadNode = SVFUtil::cast<LoadSVFGNode>(Node);
  const PAGNode *SrcNode = LoadNode->getPAGSrcNode();
  auto &PtsSet = Pta->getPts(SrcNode->getId());
  return PtsSet;
}

bool DefUseSolver::containTarget(const PointsTo &PtsSet) {
  ValueSet Targets;
  getValueSetFromPointsTo(Targets, PtsSet);
  for (auto *Target : Targets) {
    if (ProtInfo->hasTarget(Target))
      return true;
  }
  return false;
}

void DefUseSolver::getValueSetFromPointsTo(ValueSet &Values, const PointsTo &PtsSet) {
  for (auto Pts : PtsSet) {
    const PAGNode *PtsNode = Pag->getGNode(Pts);
    // const PAGNode *PtsNode = Pag->getGNode(Pag->getBaseValVar(Pts));
    if (!PtsNode->hasValue())
      continue;
    Values.insert((Value *)PtsNode->getValue());
  }
}

bool DefUseSolver::isTargetStore(NodeID ID) {
  const SVFGNode *Node = Svfg->getSVFGNode(ID);
  if (!SVFUtil::isa<StoreSVFGNode>(Node))
    return false;
  const PointsTo &PtsSet = collectStoreTarget(ID);
  return containTarget(PtsSet);
}

bool DefUseSolver::isTargetLoad(NodeID ID) {
  const SVFGNode *Node = Svfg->getSVFGNode(ID);
  if (!SVFUtil::isa<LoadSVFGNode>(Node))
    return false;
  const PointsTo &PtsSet = collectLoadTarget(ID);
  return containTarget(PtsSet);
}

void DefUseSolver::solve() {
  // initialize worklist
  for (auto It = Svfg->begin(), Eit = Svfg->end(); It != Eit; ++It) {
    NodeID ID = It->first;
    if (isTargetStore(ID))
      pushIntoWorklist(ID);
  }

  // Reaching Definitions
  NodeToDefIDMap NodeToDefs;
  while (!isWorklistEmpty()) {
    NodeID ID = Worklist.pop();
    const SVFGNode *Node = Svfg->getSVFGNode(ID);
    assert(Node != nullptr);

    DefIDVec &Facts = NodeToDefs[ID];

    // Generate def data-facts.
    if (isTargetStore(ID))
      Facts.set(ID);
    
    // Propagate data-facts to successor nodes
    for (const auto &OutEdge : Node->getOutEdges()) {
      NodeID SuccID = OutEdge->getDstID();
      if ((NodeToDefs[SuccID] |= Facts) == true)  // if there is a change
        Worklist.push(SuccID);
    }
  }

  // Create DefUse map
  DefUseIDInfo DefUse;
  for (const auto &It : NodeToDefs) {
    NodeID ID = It.first;
    const SVFGNode *Node = Svfg->getSVFGNode(ID);
    if (isTargetLoad(ID)) {
      for (NodeID DefID : It.second) {
        addDefUse(DefUse, DefID, ID);
      }
    }
  }

  // Renaming optimization: Calculate equivalent sets of Def
  std::vector<EquivalentDefSet> EquivalentDefs;
  calcEquivalentDefSet(DefUse, EquivalentDefs);

  // Register UseDef to ProtectInfo
  registerUseDef(EquivalentDefs);
}

/// Get Value * from NodeID
Value *DefUseSolver::getValue(NodeID ID) {
  const SVFGNode *Node = Svfg->getSVFGNode(ID);
  assert(Node != nullptr);
  Value *Val = (Value *)Node->getValue();
  assert(Val != nullptr);
  return Val;
}

/// Return true if two Values are data race
bool DefUseSolver::isDataRace(Value *V1, Value *V2) {
  if (Instruction *I1 = SVFUtil::dyn_cast<Instruction>(V1)) {
    if (Instruction *I2 = SVFUtil::dyn_cast<Instruction>(V2)) {
      return Mhp->mayHappenInParallel(I1, I2)
             && !LockAna->isProtectedByCommonLock(I1, I2);
    }
  }
  return false;
}

/// Add DefUse to DefUseIDInfo.
/// Conditions:
///   - Def + Use are accesses of protection targets
///   - Def + Use are race free
void DefUseSolver::addDefUse(DefUseIDInfo &DefUse, NodeID Def, NodeID Use) {
  if (!isTargetLoad(Use) || !isTargetStore(Def))
    return;
  Value *UseVal = getValue(Use);
  Value *DefVal = getValue(Def);
  NodeID UniqueID = DefUse.getUniqueID(DefVal, Def);
  if (isDataRace(UseVal, DefVal)) {
    llvm::errs() << "DataRace:\n";
    llvm::errs() << " - Use: " << *UseVal << "\n";
    llvm::errs() << " - Def: " << *DefVal << "\n";
    DefUse.insertDataRaceDefUseID(UniqueID, Use);
  } else {
    DefUse.insertDefUseID(UniqueID, Use);
  }
}

/// Register renaming optimized UseDef to ProtectInfo
void DefUseSolver::registerUseDef(std::vector<EquivalentDefSet> &EquivalentDefs) {
  // Assign DefIDs and Register them to ProtectInfo
  for (const auto &EquivDefs : EquivalentDefs) {
    DefID ID = getNextID();
    for (NodeID DefID : EquivDefs.Defs) {
      Value *Def = getValue(DefID);
      DefUseKind Kind = analyzeDefKind(DefID);
      // ProtInfo->setDefID(Def, ID);
      if (Kind.isAlignedOnlyDef())
        ProtInfo->insertAlignedOnlyDef(Def, ID);
      else if (Kind.isUnalignedOnlyDef())
        ProtInfo->insertUnalignedOnlyDef(Def, ID);
      else if (Kind.isBothOnlyDef())
        ProtInfo->insertBothOnlyDef(Def, ID);
      else if (Kind.isAlignedOrNoTargetDef())
        ProtInfo->insertAlignedOrNoTargetDef(Def, ID);
      else if (Kind.isUnalignedOrNoTargetDef())
        ProtInfo->insertUnalignedOrNoTargetDef(Def, ID);
      else if (Kind.isBothOrNoTargetDef())
        ProtInfo->insertBothOrNoTargetDef(Def, ID);
    }
    for (NodeID UseID : EquivDefs.Uses) {
      Value *Use = getValue(UseID);
      DefUseKind Kind = analyzeUseKind(UseID);
      ProtInfo->addUseDef(Use, ID);
      if (ProtInfo->hasUse(Use))
        continue;
      if (Kind.isAlignedOnlyUse())
        ProtInfo->insertAlignedOnlyUse(Use);
      else if (Kind.isUnalignedOnlyUse())
        ProtInfo->insertUnalignedOnlyUse(Use);
      else if (Kind.isBothOnlyUse())
        ProtInfo->insertBothOnlyUse(Use);
      else if (Kind.isAlignedOrNoTargetUse())
        ProtInfo->insertAlignedOrNoTargetUse(Use);
      else if (Kind.isUnalignedOrNoTargetUse())
        ProtInfo->insertUnalignedOrNoTargetUse(Use);
      else if (Kind.isBothOrNoTargetUse())
        ProtInfo->insertBothOrNoTargetUse(Use);
    }
  }
}

void DefUseSolver::registerUseDef(DefUseIDInfo &DefUse) {
  for (const auto &Iter : DefUse.DefUseID) {
    // TODO: Aligned or Unaligned check
    DefID ID = getNextID();
    Value *Def = getValue(Iter.first);
    ProtInfo->setDefID(Def, ID);
    for (NodeID UseID : Iter.second) {
      // TODO: Aligned or Unaligned check
      Value *Use = getValue(UseID);
      ProtInfo->addUseDef(Use, ID);
    }
  }
}

void DefUseSolver::calcEquivalentDefSet(DefUseIDInfo &DefUse, std::vector<EquivalentDefSet> &EquivalentDefs) {
  for (auto &Iter : DefUse.DefUseID) {
    NodeID DefID = Iter.first;
    UseIDVec &UseIDs = Iter.second;
    std::vector<EquivalentDefSet>::iterator Iter2;
    for (Iter2 = EquivalentDefs.begin(); Iter2 != EquivalentDefs.end(); ++Iter2) {
      if (Iter2->Uses == UseIDs) {
        Iter2->Defs.set(DefID);
        break;
      }
    }
    if (Iter2 == EquivalentDefs.end())
      EquivalentDefs.emplace_back(DefID, UseIDs);
  }
}

/// DefUseKind analysis
DefUseKind DefUseSolver::analyzeDefKind(NodeID ID) {
  LLVM_DEBUG(
    const Value *Store = Svfg->getSVFGNode(ID)->getValue();
    if (Store == nullptr) llvm::dbgs() << "Store: nullptr\n";
    else                  llvm::dbgs() << "Store: " << *Store << "\n";
  );
  auto &PtsSet = collectStoreTarget(ID);

  struct DefUseKind Kind;
  Kind.IsDef = true;
  setDefUseKind(Kind, PtsSet);
  return Kind;
}

DefUseKind DefUseSolver::analyzeUseKind(NodeID ID) {
  LLVM_DEBUG(
    const Value *Load = Svfg->getSVFGNode(ID)->getValue();
    if (Load == nullptr)  llvm::dbgs() << "Load: nullptr\n";
    else                  llvm::dbgs() << "Load: " << *Load << "\n";
  );
  auto &PtsSet = collectLoadTarget(ID);

  struct DefUseKind Kind;
  Kind.IsUse = true;
  setDefUseKind(Kind, PtsSet);
  return Kind;
}

void DefUseSolver::setDefUseKind(DefUseKind &Kind, const PointsTo &PtsSet) {
  ValueSet Targets;
  getValueSetFromPointsTo(Targets, PtsSet);
  for (auto *Target : Targets) {
    LLVM_DEBUG(llvm::dbgs() << " - Target: " << *Target << "\n");
    assert(Target != nullptr);
    Kind.IsAligned   |= ProtInfo->hasAlignedTarget(Target);
    Kind.IsUnaligned |= ProtInfo->hasUnalignedTarget(Target);
    Kind.IsNoTarget  |= !(ProtInfo->hasTarget(Target));
  }
  LLVM_DEBUG(llvm::dbgs() << " - Kind: " << (Kind.IsAligned ? "aligned" : "") << " "
                                         << (Kind.IsUnaligned ? "unaligned" : "") << " "
                                         << (Kind.IsNoTarget ? "no-target" : "") << "\n");
}
