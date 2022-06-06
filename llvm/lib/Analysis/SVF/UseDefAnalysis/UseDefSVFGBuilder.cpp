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
#include "MemoryModel/SVFVariables.h"
#include "Util/BasicTypes.h"

#include "llvm/Support/Debug.h"
#define DEBUG_TYPE "usedef-svfg-builder"

using namespace SVF;
using namespace SVFUtil;
using namespace llvm;

void UseDefSVFGBuilder::buildSVFG() {
  SVFIR *Pag = svfg->getPAG();
  LLVM_DEBUG(Pag->dump("pag"));

  svfg->buildSVFG();
  MemSSA *Mssa = svfg->getMSSA();
  BVDataPTAImpl *Pta = Mssa->getPTA();

  LLVM_DEBUG(svfg->dump("full-svfg"));

  mergeGlobalArrayInitializer(Pag);
  rmDerefDirSVFGEdges(Pta);
  rmIncomingEdgeForSUStore(Pta);
  rmDirOutgoingEdgeForLoad(Pta);
  rmDirEdgeFromMemcpyToMemcpy(Pta);
}

void UseDefSVFGBuilder::mergeGlobalArrayInitializer(SVFIR *Pag) {
  using ToMergeVec = llvm::SmallVector<StoreSVFGNode *, 8>;
  using ArrayToMergeVec = std::unordered_map<const GlobalVariable *, ToMergeVec>;
  ArrayToMergeVec ArrayToMerge;

  // Find global array initializations.
  for (const auto Iter : *svfg) {
    if (StoreSVFGNode *StoreNode = SVFUtil::dyn_cast<StoreSVFGNode>(Iter.second)) {
      LLVM_DEBUG(llvm::dbgs() << "StoreNode: " << StoreNode->toString() << "\n");
      NodeBS DefVars = StoreNode->getDefSVFVars();

      auto *PagSrcNode = StoreNode->getPAGSrcNode();
      auto *IcfgNode = StoreNode->getICFGNode();
      if (!PagSrcNode->isConstantData() || IcfgNode != Pag->getICFG()->getGlobalICFGNode())
        continue;

      for (auto DefVarID : DefVars) {
        SVFVar *DefVar = Pag->getGNode(DefVarID);
        LLVM_DEBUG(llvm::dbgs() << "DefVar: " << DefVar->toString() << "\n");

        if (DefVar->getNodeKind() == SVFVar::DummyValNode || DefVar->getNodeKind() == SVFVar::DummyObjNode)
          continue;

        if (const auto *GlobalVar = SVFUtil::dyn_cast<llvm::GlobalVariable>(DefVar->getValue())) {
          if (!SVFUtil::isa<llvm::ArrayType>(GlobalVar->getValueType()))
            continue;
          const auto *ArrayTy = SVFUtil::dyn_cast<llvm::ArrayType>(GlobalVar->getValueType());
          LLVM_DEBUG(llvm::dbgs() << "Global ArrayType: " << *ArrayTy << "\n");
          ArrayToMerge[GlobalVar].push_back(StoreNode);
        }
      }
    }
  }

  // Merge global array initialization nodes into one node.
  for (const auto &Iter : ArrayToMerge) {
    const auto *ArrayVal = Iter.first;
    LLVM_DEBUG(llvm::dbgs() << "Array: " << *ArrayVal << "\n");
    StoreSVFGNode *DelegateNode = nullptr;
    for (StoreSVFGNode *ToMerge : Iter.second) {
      LLVM_DEBUG(llvm::dbgs() << " - ToMerge: " << ToMerge->toString() << "\n");
      if (DelegateNode == nullptr)
        DelegateNode = ToMerge;
      else {    // Merge the node to DelegateNode.
        Set<SVFGNode *> ToAddDstSet;
        SVFGEdgeSet ToRemoveSet;
        for (auto *OutEdge : ToMerge->getOutEdges()) {
          LLVM_DEBUG(llvm::dbgs() << "    - OutEdge: " << OutEdge->toString() << "\n");
          ToAddDstSet.insert(OutEdge->getDstNode());
          ToRemoveSet.insert(OutEdge);
        }
        for (auto *ToAddDst : ToAddDstSet) {
          svfg->addIntraIndirectVFEdge(DelegateNode->getId(), ToAddDst->getId(), {});
        }
        for (auto *ToRemove : ToRemoveSet)
          svfg->removeSVFGEdge(ToRemove);
      }
    }
  }
}

void UseDefSVFGBuilder::rmDerefDirSVFGEdges(BVDataPTAImpl *Pta) {
  for (const auto Iter : *svfg) {
    const SVFGNode *Node = Iter.second;
    if (const StmtSVFGNode *StmtNode = SVFUtil::dyn_cast<StmtSVFGNode>(Node)) {
      if (SVFUtil::isa<StoreSVFGNode>(StmtNode)) {    // delete {Addr, Gep} --dir--> Store
        const SVFGNode *Def = svfg->getDefSVFGNode(StmtNode->getPAGDstNode());
        if (SVFGEdge *Edge = svfg->getIntraVFGEdge(Def, StmtNode, SVFGEdge::IntraDirectVF)) {
          LLVM_DEBUG(dbgs() << "Store delete " << Edge->toString() << "\n");
          svfg->removeSVFGEdge(Edge);
        }
      } else if (SVFUtil::isa<LoadSVFGNode>(StmtNode)) {  // delete {Addr, Gep} --dir--> Load
        const SVFGNode *Def = svfg->getDefSVFGNode(StmtNode->getPAGSrcNode());
        if (SVFGEdge *Edge = svfg->getIntraVFGEdge(Def, StmtNode, SVFGEdge::IntraDirectVF)) {
          LLVM_DEBUG(dbgs() << "Load delete " << Edge->toString() << "\n");
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
        // Remove direct edges
        for (const auto &OutEdge : LoadNode->getOutEdges())
          ToRemove.insert(OutEdge);
      }
    }
  }

  for (SVFGEdge *Edge : ToRemove) {
    svfg->removeSVFGEdge(Edge);
  }
}

void UseDefSVFGBuilder::printSVFGNodes(SVFGNode::VFGNodeK Type) const {
  assert(svfg && "svfg is nullptr!!");
  LLVM_DEBUG(dbgs() << __func__ << "(" << Type << ")\n");
  for (const auto Iter : *svfg) {
    const auto *SvfgNode = Iter.second;
    if (SvfgNode->getNodeKind() == Type) {
      LLVM_DEBUG(dbgs() << SvfgNode->toString() << "\n");
      if (const auto *StmtNode = SVFUtil::dyn_cast<StmtSVFGNode>(SvfgNode)) {
        LLVM_DEBUG(dbgs() << "Src: " << StmtNode->getPAGSrcNode()->toString() << "\n");
        LLVM_DEBUG(dbgs() << "Dst: " << StmtNode->getPAGDstNode()->toString() << "\n");
      }
    }
  }
}