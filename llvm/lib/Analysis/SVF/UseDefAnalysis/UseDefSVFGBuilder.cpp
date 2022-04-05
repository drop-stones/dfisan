#include "UseDefAnalysis/UseDefSVFGBuilder.h"
#include "MemoryModel/PointerAnalysisImpl.h"  // BVDataPTAImpl

using namespace SVF;
using namespace SVFUtil;

void UseDefSVFGBuilder::buildSVFG() {
  MemSSA *mssa = svfg->getMSSA();
  svfg->buildSVFG();
  BVDataPTAImpl *pta = mssa->getPTA();

  rmDerefDirSVFGEdges(pta);
  rmIncomingEdgeForSUStore(pta);
  rmDirOutgoingEdgeForLoad(pta);
}

void UseDefSVFGBuilder::rmDerefDirSVFGEdges(BVDataPTAImpl *pta) {
  for (const auto it : *svfg) {
    const SVFGNode *node = it.second;
    if (const StmtSVFGNode *stmtNode = SVFUtil::dyn_cast<StmtSVFGNode>(node)) {
      if (SVFUtil::isa<StoreSVFGNode>(stmtNode)) {
        const SVFGNode *def = svfg->getDefSVFGNode(stmtNode->getPAGDstNode());
        if (SVFGEdge *edge = svfg->getIntraVFGEdge(def, stmtNode, SVFGEdge::IntraDirectVF))
          svfg->removeSVFGEdge(edge);
      } else if (SVFUtil::isa<LoadSVFGNode>(stmtNode)) {
        const SVFGNode *def = svfg->getDefSVFGNode(stmtNode->getPAGSrcNode());
        if (SVFGEdge *edge = svfg->getIntraVFGEdge(def, stmtNode, SVFGEdge::IntraDirectVF))
          svfg->removeSVFGEdge(edge);
      }
    }
  }
}

bool UseDefSVFGBuilder::isStrongUpdate(const SVFGNode *node, NodeID &singleton, BVDataPTAImpl *pta) {
  bool isSU = false;
  if (const StoreSVFGNode *store = SVFUtil::dyn_cast<StoreSVFGNode>(node)) {
    const PointsTo &dstCPSet = pta->getPts(store->getPAGDstNodeID());
    if (dstCPSet.count() == 1) {
      // Find the unique element in cpts
      PointsTo::iterator it = dstCPSet.begin();
      singleton = *it;

      // Strong update can be made if this points-to target is not heap, array or field-insensitive.
      if (!pta->isHeapMemObj(singleton) && !pta->isArrayMemObj(singleton)
          && SVFIR::getPAG()->getBaseObj(singleton)->isFieldInsensitive() == false
          && !pta->isLocalVarInRecursiveFun(singleton)) {
        isSU = true;
      }
    }
  }
  return isSU;
}

void UseDefSVFGBuilder::rmIncomingEdgeForSUStore(BVDataPTAImpl *pta) {
  SVFGEdgeSet toRemove;
  for (const auto it : *svfg) {
    const SVFGNode *node = it.second;
    if (const StmtSVFGNode *stmtNode = SVFUtil::dyn_cast<StmtSVFGNode>(node)) {
      if (SVFUtil::isa<StoreSVFGNode>(stmtNode) && SVFUtil::isa<StoreInst>(stmtNode->getValue())) {
        NodeID singleton;
        if (isStrongUpdate(node, singleton, pta)) {
          for (const auto inEdge : node->getInEdges()) {
            if (inEdge->isIndirectVFGEdge()) {
              toRemove.insert(inEdge);
            }
          }
        }
      }
    }
  }

  for (SVFGEdge *edge : toRemove) {
    svfg->removeSVFGEdge(edge);
  }
}

void UseDefSVFGBuilder::rmDirOutgoingEdgeForLoad(BVDataPTAImpl *pta) {
  SVFGEdgeSet toRemove;
  for (const auto it : *svfg) {
    const SVFGNode *node = it.second;
    if (const StmtSVFGNode *stmtNode = SVFUtil::dyn_cast<StmtSVFGNode>(node)) {
      if (SVFUtil::isa<LoadSVFGNode>(stmtNode) && SVFUtil::isa<LoadInst>(stmtNode->getValue())) {
        for (const auto outEdge : stmtNode->getOutEdges()) {
          toRemove.insert(outEdge);
        }
      }
    }
  }

  for (SVFGEdge *edge : toRemove) {
    svfg->removeSVFGEdge(edge);
  }
}