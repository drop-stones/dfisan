//===-- UseDefAnalysis.cpp - Use-Def Analysis Implementation---------------===//
//
//===----------------------------------------------------------------------===//
///
/// This file contains the implementation of the UseDefAnalysis class,
/// which create a SVFG, analyze it, and save the results to UseDefChain class.
///
//===----------------------------------------------------------------------===//

#include "UseDefAnalysis/UseDefAnalysis.h"
#include "SVF-FE/SVFIRBuilder.h"
#include "WPA/Andersen.h"

#include "llvm/Support/Debug.h"
#define DEBUG_TYPE "usedef-analysis"

using namespace SVF;
using namespace SVFUtil;

namespace {
/// Return true if the DEF statement uses pointer or array access.
bool isDefUsingPtr(const StoreSVFGNode *Store) {
  const auto *Inst = Store->getInst();
  if (Inst == nullptr)
    return false;

  if (const auto *StoreInst = llvm::dyn_cast<const llvm::StoreInst>(Inst)) {
    const auto *StoreAddr = StoreInst->getPointerOperand();
    if (llvm::isa<LoadInst>(StoreAddr) || llvm::isa<GetElementPtrInst>(StoreAddr)) {
      return true;
    }
  }
  return false;
}

/// Return true if the DEF statment is memcpy.
bool isMemcpy(const StmtSVFGNode *Store) {
  const auto *Inst = Store->getInst();
  if (Inst == nullptr)
    return false;
  
  if (llvm::isa<const llvm::MemCpyInst>(Inst))
    return true;
  
  return false;
}

const SVFVar *getSVFVar(const SVFG *Svfg, const StoreSVFGNode *Store) {
  SVFIR *Pag = Svfg->getPAG();
  const auto *DstNode = Svfg->getDefSVFGNode(Store->getPAGDstNode());
  if (const auto *GepNode = SVFUtil::dyn_cast<const GepSVFGNode>(DstNode)) {
    const auto DefVars = GepNode->getDefSVFVars();
    for (const auto Field : DefVars) {
      return Pag->getGNode(Field);
    }
  }
  return nullptr;
}

const SVFVar *getSVFVar(const SVFG *Svfg, const LoadSVFGNode *Load) {
  SVFIR *Pag = Svfg->getPAG();
  const auto *SrcNode = Svfg->getDefSVFGNode(Load->getPAGSrcNode());
  if (const auto *GepNode = SVFUtil::dyn_cast<const GepSVFGNode>(SrcNode)) {
    const auto DefVars = GepNode->getDefSVFVars();
    for (const auto Field : DefVars) {
      return Pag->getGNode(Field);
    }
  }
  return nullptr;
}
} // namespace

/// Initialize analysis
void UseDefAnalysis::initialize(SVFModule *M) {
  IRBuilder.build(M);
  Pag = IRBuilder.getPAG();

  AndersenWaveDiff *Ander = AndersenWaveDiff::createAndersenWaveDiff(Pag);
  Svfg = SvfgBuilder.buildFullSVFG(Ander);
  UseDef = new UseDefChain(getModuleName(M));
}

/// Start analysis.
void UseDefAnalysis::analyze(SVFModule *M) {
  initialize(M);

  // Create GlobalInitList.
  for (const auto *Iter : Svfg->getGlobalVFGNodes()) {
    if (const auto *StoreNode = SVFUtil::dyn_cast<StoreSVFGNode>(Iter)) {
      UseDef->insertGlobalInit(StoreNode);
      UseDef->insertFieldStore(Svfg, StoreNode);  // Insert if the value is global struct.
    }
  }

  // Push all StoreSVFGNodes to worklist.
  FIFOWorkList<NodeID> Worklist;
  for (const auto &Iter : *Svfg) {
    const NodeID ID = Iter.first;
    const SVFGNode *Node = Iter.second;
    if (const auto *StoreNode = SVFUtil::dyn_cast<StoreSVFGNode>(Node)) {
      Worklist.push(ID);
      if (isDefUsingPtr(StoreNode))
        UseDef->insertDefUsingPtr(StoreNode);
      if (isMemcpy(StoreNode))
        UseDef->insertFieldStore(Svfg, StoreNode);
    }
  }

  using DefIDSet = llvm::SparseBitVector<>;
  using SVFGNodeToStoreMap = std::unordered_map<NodeID, DefIDSet>;
  SVFGNodeToStoreMap NodeToDefs;  // Map from nodes to out data-facts.
  while (!Worklist.empty()) {
    const NodeID ID = Worklist.pop();
    const SVFGNode *Node = Svfg->getSVFGNode(ID);
    assert(Node != nullptr);

    DefIDSet &Facts = NodeToDefs[ID];

    /// Add an Def Data-fact.
    if (const auto *StoreNode = SVFUtil::dyn_cast<StoreSVFGNode>(Node)) {
      Facts.set(ID);
    }

    /// Propagate Data-Facts to successor Nodes.
    for (const auto &OutEdge : Node->getOutEdges()) {
      const NodeID SuccID = OutEdge->getDstID();
      if ((NodeToDefs[SuccID] |= Facts) == true) {  // If there is a change
        Worklist.push(SuccID);
      }
    }
  }

  // Create Use-Def map from analysis results.
  for (const auto &Iter : NodeToDefs) {
    const NodeID UseID = Iter.first;
    if (const auto *UseNode = SVFUtil::dyn_cast<LoadSVFGNode>(Svfg->getSVFGNode(UseID))) {
      if (UseDef->isPaddingArray(getSVFVar(Svfg, UseNode))) {
        LLVM_DEBUG(llvm::dbgs() << "Skip Use of padding array: " << UseNode->toString() << "\n");
        continue;
      }
      for (const NodeID DefID : Iter.second) {
        if (const auto *DefNode = SVFUtil::dyn_cast<StoreSVFGNode>(Svfg->getSVFGNode(DefID))) {
          if (UseDef->isPaddingArray(getSVFVar(Svfg, DefNode))) {
            LLVM_DEBUG(llvm::dbgs() << "Skip Def of padding array: " << DefNode->toString() << "\n");
            continue;
          }
          UseDef->insert(UseNode, DefNode);
        }
      }
    }
  }

  // ID to Defs
  UseDef->idToUseDef(Pag);
}