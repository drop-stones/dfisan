#include "UseDefAnalysis/UseDefSVFGBuilder.h"
//#include "Graphs/SVFG.h"

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
  // TODO
}

bool UseDefSVFGBuilder::isStrongUpdate(const SVFGNode *node, NodeID &singleton, BVDataPTAImpl *pta) {
  bool isSU = false;
  return isSU;
}

void UseDefSVFGBuilder::rmIncomingEdgeForSUStore(BVDataPTAImpl *pta) {
  // TODO
}

void UseDefSVFGBuilder::rmDirOutgoingEdgeForLoad(BVDataPTAImpl *pta) {
  // TODO
}