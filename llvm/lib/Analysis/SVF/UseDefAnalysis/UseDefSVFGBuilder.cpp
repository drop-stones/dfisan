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
  addGlobalAggregateTypeInitialization(Pag);

  svfg->buildSVFG();
  MemSSA *Mssa = svfg->getMSSA();
  BVDataPTAImpl *Pta = Mssa->getPTA();

  LLVM_DEBUG(svfg->dump("full-svfg"));

  rmDerefDirSVFGEdges(Pta);
  rmIncomingEdgeForSUStore(Pta);
  rmDirOutgoingEdgeForLoad(Pta);
  rmDirEdgeFromMemcpyToMemcpy(Pta);
}

// TODO: Support global struct zeroinitializer
void UseDefSVFGBuilder::addGlobalStructZeroInitializer(SVFIR *Pag, const llvm::GlobalVariable *GlobalVar) {
  const auto *StructTy = SVFUtil::dyn_cast<llvm::StructType>(GlobalVar->getType());
  const auto *StructInit = GlobalVar->getInitializer();
  if (!StructInit->isZeroValue())
    return;
  LLVM_DEBUG(dbgs() << "Struct: " << *GlobalVar << "\n");
  LLVM_DEBUG(dbgs() << "StructInit: " << *StructInit << "\n");
  LLVM_DEBUG(dbgs() << "IsZero = " << StructInit->isZeroValue() << "\n");

  return;
  for (auto *EleTy : StructTy->elements()) {
    if (auto *StructTy = SVFUtil::dyn_cast<llvm::StructType>(EleTy)) {
      //addGlobalStructZeroInitializer(Pag, )
    } else if (auto *ArrayTy = SVFUtil::dyn_cast<llvm::ArrayType>(EleTy)) {
      //ConstantAggregateZero *ZeroInit = new ConstantAggregateZero (ArrayTy);
    } else if (auto *IntegerTy = SVFUtil::dyn_cast<llvm::IntegerType>(EleTy)) {
      //ConstantInt *Const = new ConstantInt (IntegerTy, 0);
    }
  }
}

// TODO: Support array in struct
void UseDefSVFGBuilder::addGlobalAggregateTypeInitialization(SVFIR *Pag) {
  LLVM_DEBUG(dbgs() << __func__ << "\n");
  llvm::SmallVector<const llvm::GlobalVariable *, 8> ArrayVec;
  llvm::SmallVector<const llvm::GlobalVariable *, 8> StructVec;
  for (const auto *GlobalNode : svfg->getGlobalVFGNodes()) {
    if (! SVFUtil::isa<AddrVFGNode>(GlobalNode))
      continue;
    const AddrVFGNode *AddrNode = SVFUtil::dyn_cast<AddrVFGNode>(GlobalNode);
    const auto *Val = AddrNode->getValue();
    if (Val == nullptr || !SVFUtil::isa<llvm::GlobalVariable>(Val))
      continue;
    const auto *GlobalVar = SVFUtil::dyn_cast<llvm::GlobalVariable>(Val);
    const auto *VarTy = GlobalVar->getValueType();
    if (VarTy == nullptr || (!SVFUtil::isa<llvm::ArrayType>(VarTy) && !SVFUtil::isa<llvm::StructType>(VarTy)))
      continue;
    if (SVFUtil::isa<llvm::ArrayType>(VarTy))
      ArrayVec.push_back(GlobalVar);
    if (SVFUtil::isa<llvm::StructType>(VarTy))
      StructVec.push_back(GlobalVar);
  }

  for (const auto *GlobalVar : ArrayVec) {
    const auto *ArrayTy = SVFUtil::dyn_cast<llvm::ArrayType>(GlobalVar->getType());
    const auto *ArrayInit = GlobalVar->getInitializer();
    LLVM_DEBUG(dbgs() << "Array: " << *GlobalVar << "\n");
    LLVM_DEBUG(dbgs() << "ArrayInit: " << *ArrayInit << "\n");
    LLVM_DEBUG(dbgs() << "IsZero = " << ArrayInit->isZeroValue() << "\n");
    NodeID SrcID = Pag->getPAGNodeNum();
    Pag->addValNode(ArrayInit, SrcID);
    const auto *Src = Pag->getGNode(SrcID);
    NodeID DstID = Pag->getValueNode(GlobalVar);
    const auto *Dst = Pag->getGNode(DstID);
    // TODO: Dst must be all field (or base) of struct
    LLVM_DEBUG(dbgs() << "Create new StoreStmt:\n");
    LLVM_DEBUG(dbgs() << "Src: " << Src->toString() << "\n");
    LLVM_DEBUG(dbgs() << "Dst: " << Dst->toString() << "\n");

    auto *Stmt = Pag->addStoreStmt(SrcID, DstID, nullptr);
    Stmt->setICFGNode(Pag->getICFG()->getGlobalICFGNode());
    Stmt->setValue(ArrayInit);
    LLVM_DEBUG(dbgs() << "StoreStmt: " << Stmt->toString() << "\n");

    svfg->addStoreVFGNode(Stmt);
  }

  for (const auto *GlobalVar : StructVec) {
    // TODO: support zeroinitializer, internal structs
    addGlobalStructZeroInitializer(Pag, GlobalVar);
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