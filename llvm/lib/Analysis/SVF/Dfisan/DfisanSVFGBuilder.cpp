#include "Dfisan/DfisanSVFGBuilder.h"
#include "Dfisan/DfisanSVFG.h"
#include "Dfisan/DfisanMemSSA.h"
#include "Util/Options.h"

using namespace SVF;
using namespace SVFUtil;

SVFG *DfisanSVFGBuilder::buildFullSVFG(BVDataPTAImpl *pta) {
  return build(pta, VFG::FULLSVFG);
}

SVFG *DfisanSVFGBuilder::build(BVDataPTAImpl *pta, VFG::VFGK kind) {
  assert(kind == VFG::VFGK::FULLSVFG && "DfsanSVFG must be full-svfg");
  MemSSA *mssa = buildMSSA(pta, false);
  svfg = new DfisanSVFG(mssa, kind);
  buildSVFG();

  if (mssa->getPTA()->printStat())
    svfg->performStat();
  if (Options::DumpVFG)
    svfg->dump("svfg_final");
  
  return svfg;
}

MemSSA *DfisanSVFGBuilder::buildMSSA(BVDataPTAImpl *pta, bool ptrOnlyMSSA) {
  assert(ptrOnlyMSSA == false);

  // Replace MemSSA with DfisanMemSSA.
  DfisanMemSSA *mssa = new DfisanMemSSA(pta, ptrOnlyMSSA);

  DominatorTree dt;
  MemSSADF df;

  SVFModule *svfModule = mssa->getPTA()->getModule();
  for (SVFModule::const_iterator iter = svfModule->begin(), eiter = svfModule->end();
       iter != eiter; ++iter) {
    const SVFFunction *fun = *iter;
    if (isExtCall(fun))
      continue;
    
    dt.recalculate(*fun->getLLVMFun());
    df.runOnDT(dt);

    mssa->buildMemSSA(*fun, &df, &dt);
  }

  if (mssa->getPTA()->printStat())
    mssa->performStat();
  if (Options::DumpMSSA) {
    mssa->dumpMSSA();
  }

  return mssa;
}

void DfisanSVFGBuilder::buildSVFG() {
  MTASVFGBuilder::buildSVFG();

  BVDataPTAImpl *Pta = svfg->getMSSA()->getPTA();
  rmIncomingEdgeForSUStore(Pta);
  rmAllDirSVFGEdge();
}

bool DfisanSVFGBuilder::isStrongUpdate(const SVFGNode *Node, NodeID &Singleton, BVDataPTAImpl *Pta) {
  bool IsSU = false;
  if (const StoreSVFGNode *Store = SVFUtil::dyn_cast<StoreSVFGNode>(Node)) {
    const PointsTo &DstCPSet = Pta->getPts(Store->getPAGDstNodeID());
    if (DstCPSet.count() == 1) {
      // Find the unique element in cpts
      PointsTo::iterator Iter = DstCPSet.begin();
      Singleton = *Iter;

      // Strong update can be made if this points-to target is not array or field-insensitive.
      // ?? Heap or Local in recursive func can be strong updated ??
      if (!Pta->isArrayMemObj(Singleton)
          && SVFIR::getPAG()->getBaseObj(Singleton)->isFieldInsensitive() == false)
        IsSU = true;
    }
  }
  return IsSU;
}

void DfisanSVFGBuilder::rmIncomingEdgeForSUStore(BVDataPTAImpl *Pta) {
  for (SVFG::iterator It = svfg->begin(), Eit = svfg->end(); It != Eit; ++It) {
    const SVFGNode *Node = It->second;
    if (const StoreSVFGNode *StoreNode = SVFUtil::dyn_cast<StoreSVFGNode>(Node)) {
      if (SVFUtil::isa<StoreInst>(StoreNode->getValue())) {
        NodeID Singleton;
        if (isStrongUpdate(StoreNode, Singleton, Pta)) {
          Set<SVFGEdge *> ToRemove;
          for (SVFGNode::const_iterator It2 = StoreNode->InEdgeBegin(), Eit2 = StoreNode->InEdgeEnd(); It2 != Eit2; ++It2) {
            if ((*It2)->isIndirectVFGEdge())
              ToRemove.insert(*It2);
          }
          for (SVFGEdge *Edge : ToRemove)
            svfg->removeSVFGEdge(Edge);
        }
      }
    }
  }
}

void DfisanSVFGBuilder::rmAllDirSVFGEdge() {
  Set<SVFGEdge *> ToRemove;
  for (SVFG::iterator It = svfg->begin(), Eit = svfg->end(); It != Eit; ++It) {
    const SVFGNode *Node = It->second;
    for (SVFGNode::const_iterator It2 = Node->InEdgeBegin(), Eit2 = Node->InEdgeEnd(); It2 != Eit2; ++It2)
      if ((*It2)->isDirectVFGEdge())
        ToRemove.insert(*It2);
    for (SVFGNode::const_iterator It2 = Node->OutEdgeBegin(), Eit2 = Node->OutEdgeEnd(); It2 != Eit2; ++It2)
      if ((*It2)->isDirectVFGEdge())
        ToRemove.insert(*It2);
  }
  for (SVFGEdge *Edge : ToRemove)
    svfg->removeSVFGEdge(Edge);
}
