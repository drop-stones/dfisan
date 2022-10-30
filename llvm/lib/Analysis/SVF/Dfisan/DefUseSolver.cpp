#include "MTA/MHP.h"
#include "MTA/LockAnalysis.h"
#include "Dfisan/DefUseSolver.h"

using namespace SVF;

bool isTargetStore(const SVFGNode *Node) {
  // TODO: check store target
  return SVFUtil::isa<StoreSVFGNode>(Node);
}

bool isTargetLoad(const SVFGNode *Node) {
  // TODO: check load target
  return SVFUtil::isa<LoadSVFGNode>(Node);
}

void DefUseSolver::solve() {
  // initialize worklist
  for (auto It = Svfg->begin(), Eit = Svfg->end(); It != Eit; ++It) {
    NodeID ID = It->first;
    const SVFGNode *Node = It->second;
    if (isTargetStore(Node))
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
    if (isTargetStore(Node))
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
    if (isTargetLoad(Node)) {
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
  const SVFGNode *UseNode = Svfg->getSVFGNode(Use);
  const SVFGNode *DefNode = Svfg->getSVFGNode(Def);
  if (!isTargetLoad(UseNode) || !isTargetStore(DefNode))
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
      // TODO: Aligned or Unaligned check
      Value *Def = getValue(DefID);
      ProtInfo->insertDef(Def, ID);
    }
    for (NodeID UseID : EquivDefs.Uses) {
      // TODO: Aligned or Unaligned check
      Value *Use = getValue(UseID);
      ProtInfo->addUseDef(Use, ID);
    }
  }
}

void DefUseSolver::registerUseDef(DefUseIDInfo &DefUse) {
  for (const auto &Iter : DefUse.DefUseID) {
    DefID ID = getNextID();
    // TODO: Aligned or Unaligned check
    Value *Def = getValue(Iter.first);
    ProtInfo->insertDef(Def, ID);
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
