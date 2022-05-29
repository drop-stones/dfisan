//===-- UseDefChain.cpp - Use-Def Chain implementation --------------------===//
//
//===----------------------------------------------------------------------===//
///
/// This file contains the implementation of the UseDefSVFGBuilder class,
/// which creates an SVFG and removes some unrelated edges.
///
//===----------------------------------------------------------------------===//

#include "UseDefAnalysis/UseDefSVFGBuilder.h"
#include "MemoryModel/PointerAnalysisImpl.h"  // BVDataPTAImpl

using namespace SVF;
using namespace SVFUtil;

void UseDefSVFGBuilder::buildSVFG() {
  MemSSA *Mssa = svfg->getMSSA();
  svfg->buildSVFG();
  // dump for debug
  svfg->dump("full-usedef-svfg");
  BVDataPTAImpl *Pta = Mssa->getPTA();

  rmDerefDirSVFGEdges(Pta);
  rmIncomingEdgeForSUStore(Pta);
  rmDirOutgoingEdgeForLoad(Pta);
  rmDirEdgeFromMemcpyToMemcpy(Pta);
}

void UseDefSVFGBuilder::rmDerefDirSVFGEdges(BVDataPTAImpl *Pta) {
  for (const auto Iter : *svfg) {
    const SVFGNode *Node = Iter.second;
    if (const StmtSVFGNode *StmtNode = SVFUtil::dyn_cast<StmtSVFGNode>(Node)) {
      if (SVFUtil::isa<StoreSVFGNode>(StmtNode)) {    // delete {Addr, Gep} --dir--> Store
        const SVFGNode *Def = svfg->getDefSVFGNode(StmtNode->getPAGDstNode());
        if (SVFGEdge *Edge = svfg->getIntraVFGEdge(Def, StmtNode, SVFGEdge::IntraDirectVF)) {
          //llvm::outs() << "Store delete " << Edge->toString() << "\n";
          svfg->removeSVFGEdge(Edge);
        }
      } else if (SVFUtil::isa<LoadSVFGNode>(StmtNode)) {  // delete {Addr, Gep} --dir--> Load
        const SVFGNode *Def = svfg->getDefSVFGNode(StmtNode->getPAGSrcNode());
        if (SVFGEdge *Edge = svfg->getIntraVFGEdge(Def, StmtNode, SVFGEdge::IntraDirectVF)) {
          //llvm::outs() << "Load delete " << Edge->toString() << "\n";
          svfg->removeSVFGEdge(Edge);
        }
      }
    }
  }
}

bool UseDefSVFGBuilder::isStrongUpdate(const SVFGNode *Node, NodeID &Singleton, BVDataPTAImpl *Pta) {
  bool IsSU = false;
  if (const StoreSVFGNode *Store = SVFUtil::dyn_cast<StoreSVFGNode>(Node)) {
    const PointsTo &DstCPSet = Pta->getPts(Store->getPAGDstNodeID());
    if (DstCPSet.count() == 1) {
      // Find the unique element in cpts
      PointsTo::iterator Iter = DstCPSet.begin();
      Singleton = *Iter;

      // Strong update can be made if this points-to target is not heap, array or field-insensitive.
      if (!Pta->isHeapMemObj(Singleton) && !Pta->isArrayMemObj(Singleton)
          && SVFIR::getPAG()->getBaseObj(Singleton)->isFieldInsensitive() == false
          && !Pta->isLocalVarInRecursiveFun(Singleton)) {
        IsSU = true;
      }
    }
  }
  return IsSU;
}

void UseDefSVFGBuilder::rmIncomingEdgeForSUStore(BVDataPTAImpl *Pta) {
  SVFGEdgeSet ToRemove;
  for (const auto Iter : *svfg) {
    const SVFGNode *Node = Iter.second;
    if (const StmtSVFGNode *StmtNode = SVFUtil::dyn_cast<StmtSVFGNode>(Node)) {
      if (SVFUtil::isa<StoreSVFGNode>(StmtNode) && SVFUtil::isa<StoreInst>(StmtNode->getValue())) {
        NodeID Singleton;
        if (isStrongUpdate(Node, Singleton, Pta)) {
          for (const auto &InEdge : Node->getInEdges()) {
            if (InEdge->isIndirectVFGEdge()) {
              ToRemove.insert(InEdge);
            }
          }
        }
      }
    }
  }

  for (SVFGEdge *Edge : ToRemove) {
    svfg->removeSVFGEdge(Edge);
  }
}

void UseDefSVFGBuilder::rmDirOutgoingEdgeForLoad(BVDataPTAImpl *Pta) {
  SVFGEdgeSet ToRemove;
  for (const auto Iter : *svfg) {
    const SVFGNode *Node = Iter.second;
    if (const StmtSVFGNode *StmtNode = SVFUtil::dyn_cast<StmtSVFGNode>(Node)) {
      if (SVFUtil::isa<LoadSVFGNode>(StmtNode) && SVFUtil::isa<LoadInst>(StmtNode->getValue())) {
        for (const auto &OutEdge : StmtNode->getOutEdges()) {
          ToRemove.insert(OutEdge);
        }
      }
    }
  }

  for (SVFGEdge *Edge : ToRemove) {
    svfg->removeSVFGEdge(Edge);
  }
}

void UseDefSVFGBuilder::rmDirEdgeFromMemcpyToMemcpy(BVDataPTAImpl *Pta) {
  SVFGEdgeSet ToRemove;
  for (const auto Iter : *svfg) {
    const SVFGNode *Node = Iter.second;
    if (const LoadSVFGNode *LoadNode = SVFUtil::dyn_cast<LoadSVFGNode>(Node)) {
      const auto *Inst = LoadNode->getInst();
      if (Inst == nullptr)
        continue;
      
      if (const auto *Memcpy = llvm::dyn_cast<const llvm::MemCpyInst>(Inst)) {
        // remove direct edges
        for (const auto &OutEdge : LoadNode->getOutEdges())
          ToRemove.insert(OutEdge);
      }
    }
  }

  for (SVFGEdge *Edge : ToRemove) {
    svfg->removeSVFGEdge(Edge);
  }
}