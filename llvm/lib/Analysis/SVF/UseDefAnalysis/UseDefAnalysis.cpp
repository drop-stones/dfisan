#include "UseDefAnalysis/UseDefAnalysis.h"
#include "SVF-FE/SVFIRBuilder.h"
#include "WPA/Andersen.h"

using namespace SVF;
using namespace SVFUtil;

/// Initialize analysis
void UseDefAnalysis::initialize(SVFModule *M) {
  SVFIRBuilder Builder;
  SVFIR *Pag = Builder.build(M);

  AndersenWaveDiff *Ander = AndersenWaveDiff::createAndersenWaveDiff(Pag);
  Svfg = SvfgBuilder.buildFullSVFG(Ander);
  UseDef = new UseDefChain;
}

/// Start analysis.
void UseDefAnalysis::analyze(SVFModule *M) {
  initialize(M);

  // Analyze SVFG for Use-Def calculation.
  FIFOWorkList<NodeID> worklist;
  for (const auto &it : *Svfg) {
    const NodeID id = it.first;
    const SVFGNode *node = it.second;
    if (SVFUtil::isa<StoreSVFGNode>(node))
      worklist.push(id);
  }

  using DefIDSet = llvm::SparseBitVector<>;
  using SVFGNodeToStoreMap = std::unordered_map<NodeID, DefIDSet>;
  SVFGNodeToStoreMap nodeToDefs;
  while (!worklist.empty()) {
    const NodeID id = worklist.pop();
    const SVFGNode *node = Svfg->getSVFGNode(id);
    assert(node != nullptr);

    DefIDSet &facts = nodeToDefs[id];

    /// Add an Def Data-fact.
    if (const auto *storeNode = dyn_cast<StoreSVFGNode>(node)) {
      facts.set(id);
    }

    /// Propagate Data-facts to successor nodes.
    for (const auto &outEdge : node->getOutEdges()) {
      const NodeID succId = outEdge->getDstID();
      if ((nodeToDefs[succId] |= facts) == true) {  // If there is a change
        worklist.push(succId);
      }
    }
  }

  // Create Use-Def map from analysis results.
  for (const auto &it : nodeToDefs) {
    const NodeID id = it.first;
    const SVFGNode *node = Svfg->getSVFGNode(id);
    if (const auto *useNode = dyn_cast<LoadSVFGNode>(node)) {
      for (const NodeID defID : it.second) {
        const SVFGNode *mayDefNode = Svfg->getSVFGNode(defID);
        if (const auto *defNode = dyn_cast<StoreSVFGNode>(mayDefNode)) {
          UseDef->insert(useNode, defNode);
        }
      }
    }
  }
}