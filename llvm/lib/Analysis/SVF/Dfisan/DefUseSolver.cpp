#include "MTA/MHP.h"
#include "MTA/LockAnalysis.h"
#include "Dfisan/DefUseSolver.h"
#include "Dfisan/DfisanExtAPI.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Debug.h"
#define DEBUG_TYPE "def-use-solver"

using namespace SVF;

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
  // And add operand information to ProtectInfo
  DefUseIDInfo DefUse;
  for (const auto &It : NodeToDefs) {
    NodeID ID = It.first;
    if (isTargetLoad(ID)) {
      for (NodeID DefID : It.second) {
        addDefUse(DefUse, DefID, ID);

        addDefOperand(DefID);
        addUseOperand(ID);
      }
    }
  }
  for (const auto &It : NodeToDefs) {
    NodeID ID = It.first;
    if (isTargetStore(ID)) {
      addUnusedDef(DefUse, ID);

      addDefOperand(ID);
    }
  }

  // Renaming optimization: Calculate equivalent sets of Def
  std::vector<EquivalentDefSet> EquivalentDefs;
  calcEquivalentDefSet(DefUse, EquivalentDefs);

  // Register UseDef to ProtectInfo
  registerUseDef(EquivalentDefs);
}

void DefUseSolver::collectUnsafeInst() {
  for (auto It = Svfg->begin(), Eit = Svfg->end(); It != Eit; ++It) {
    NodeID ID = It->first;
    const SVFGNode *Node = It->second;
    if (isTargetStore(ID) || isTargetLoad(ID))
      continue;
    if (const StoreSVFGNode *StoreNode = SVFUtil::dyn_cast<StoreSVFGNode>(Node)) {
      Instruction *Store = (Instruction *)StoreNode->getInst();
      AccessOperand Ope = getStoreOperand(ID);
      if (Ope.empty()) continue;
      if (Store != nullptr &&
          isUnsafeOperand(Ope) &&
          DfisanExtAPI::getDfisanExtAPI()->isCallocCall(Store) == false)
        ProtInfo->addUnsafeOperand(Store, Ope);
    } else if (const LoadSVFGNode *LoadNode = SVFUtil::dyn_cast<LoadSVFGNode>(Node)) {
      Instruction *Load = (Instruction *)LoadNode->getInst();
      AccessOperand Ope = getLoadOperand(ID);
      if (Load != nullptr && isUnsafeOperand(Ope))
        ProtInfo->addUnsafeOperand(Load, Ope);
    }
  }
}

/// Get Value * from NodeID
Value *DefUseSolver::getValue(NodeID ID) {
  const SVFGNode *Node = Svfg->getSVFGNode(ID);
  assert(Node != nullptr);
  Value *Val = (Value *)Node->getValue();
  assert(Val != nullptr);
  return Val;
}

/// Return true if two Values are alias
/// If two Values are GetElementPtrInst,
/// we distinguish the head of struct and the entire struct.
AliasResult DefUseSolver::isAlias(Value *V1, Value *V2) {
  if (llvm::isa<GetElementPtrInst>(V1) && llvm::isa<GetElementPtrInst>(V2)) {
    PointsTo Pts1 = Pta->getPts(Pag->getValueNode(V1));
    PointsTo Pts2 = Pta->getPts(Pag->getValueNode(V2));
    if (Pta->containBlackHoleNode(Pts1) || Pta->containBlackHoleNode(Pts2) || Pts1.intersects(Pts2))
      return AliasResult::MayAlias;
    return AliasResult::NoAlias;
  }
  return Pta->alias(V1, V2);
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
  Value *DefOpe = getStoreOperand(Def).Operand;
  Value *UseOpe = getLoadOperand(Use).Operand;
  if (!isAlias(DefOpe, UseOpe)) {
    LLVM_DEBUG(llvm::dbgs() << "No Alias:" << "\n");
    LLVM_DEBUG(llvm::dbgs() << " - Def: " << *DefOpe << "\n");
    LLVM_DEBUG(llvm::dbgs() << " - Use: " << *UseOpe << "\n");
    return;
  }
  Value *UseVal = getValue(Use);
  Value *DefVal = getValue(Def);
  NodeID UniqueID = getUniqueID(DefVal, Def);
  if (isDataRace(UseVal, DefVal)) {
    llvm::errs() << "DataRace:\n";
    llvm::errs() << " - Use: " << *UseVal << "\n";
    llvm::errs() << " - Def: " << *DefVal << "\n";
    DefUse.insertDataRaceDefUseID(UniqueID, Use);
  } else {
    DefUse.insertDefUseID(UniqueID, Use);
  }
}

void DefUseSolver::addUnusedDef(DefUseIDInfo &DefUse, NodeID Def) {
  Value *DefVal = getValue(Def);
  NodeID UniqueID = getUniqueID(DefVal, Def);
  if (!DefUse.hasDef(UniqueID)) {
    DefUse.insertUnusedDefID(UniqueID);
  }
}

void DefUseSolver::addDefOperand(NodeID ID) {
  Value *Def = getValue(ID);
  AccessOperand Ope = getStoreOperand(ID);
  if (Ope.empty()) return;
  if (isGlobalInit(ID))  // GlobalInit must have unique key
    ProtInfo->addDefOperand(Ope.Operand, Ope);
  else
    ProtInfo->addDefOperand(Def, Ope);
}

void DefUseSolver::addUseOperand(NodeID ID) {
  Value *Use = getValue(ID);
  AccessOperand Ope = getLoadOperand(ID);
  ProtInfo->addUseOperand(Use, Ope);
}

/// Register renaming optimized UseDef to ProtectInfo
void DefUseSolver::registerUseDef(std::vector<EquivalentDefSet> &EquivalentDefs) {
  // Assign DefIDs and Register them to ProtectInfo
  for (const auto &EquivDefs : EquivalentDefs) {
    DefID ID = getNextID();
    for (NodeID DefID : EquivDefs.Defs) {
      Value *Def = getValue(DefID);
      if (isGlobalInit(DefID)) {
        AccessOperand Ope = getStoreOperand(DefID);
        Def = Ope.Operand;
      }
      ProtInfo->setDefID(Def, ID);
      DefUseKind Kind = analyzeDefKind(DefID);
      if (Kind.isAlignedOnlyDef())
        ProtInfo->insertAlignedOnlyDef(Def);
      else if (Kind.isUnalignedOnlyDef())
        ProtInfo->insertUnalignedOnlyDef(Def);
      else if (Kind.isBothOnlyDef())
        ProtInfo->insertBothOnlyDef(Def);
      else if (Kind.isAlignedOrNoTargetDef())
        ProtInfo->insertAlignedOrNoTargetDef(Def);
      else if (Kind.isUnalignedOrNoTargetDef())
        ProtInfo->insertUnalignedOrNoTargetDef(Def);
      else if (Kind.isBothOrNoTargetDef())
        ProtInfo->insertBothOrNoTargetDef(Def);
    }
    for (NodeID UseID : EquivDefs.Uses) {
      Value *Use = getValue(UseID);
      ProtInfo->addUseDef(Use, ID);
      DefUseKind Kind = analyzeUseKind(UseID);
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
  EquivalentDefs.emplace_back(DefUse.UnusedDefIDs, UseIDVec());   // push unused defs
}

/// Access target analysis
const PointsTo &DefUseSolver::collectStoreTarget(NodeID ID) {
  const SVFGNode *Node = Svfg->getSVFGNode(ID);
  assert(SVFUtil::isa<StoreSVFGNode>(Node));
  const StoreSVFGNode *StoreNode = SVFUtil::cast<StoreSVFGNode>(Node);
  NodeID DstID = StoreNode->getPAGDstNodeID();
  auto &PtsSet = Pta->getPts(DstID);
  return PtsSet;
}

const PointsTo &DefUseSolver::collectLoadTarget(NodeID ID) {
  const SVFGNode *Node = Svfg->getSVFGNode(ID);
  assert(SVFUtil::isa<LoadSVFGNode>(Node));
  const LoadSVFGNode *LoadNode = SVFUtil::cast<LoadSVFGNode>(Node);
  NodeID SrcID = LoadNode->getPAGSrcNodeID();
  auto &PtsSet = Pta->getPts(SrcID);
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

/// Access operand analysis
AccessOperand DefUseSolver::getStoreOperand(NodeID ID) {
  const SVFGNode *Node = Svfg->getSVFGNode(ID);
  assert(SVFUtil::isa<StoreSVFGNode>(Node));
  const StoreSVFGNode *StoreNode = SVFUtil::cast<StoreSVFGNode>(Node);
  Value *Store = (Value *)StoreNode->getValue();
  const PAGNode *DstNode = StoreNode->getPAGDstNode();
  if (Store == nullptr || !DstNode->hasValue())
    return AccessOperand();
  Value *Dst = (Value *)DstNode->getValue();
  LLVM_DEBUG(llvm::dbgs() << "Store: " << *Store << "\n");
  LLVM_DEBUG(llvm::dbgs() << " - Dst: " << *Dst << "\n");
  DfisanExtAPI *ExtAPI = DfisanExtAPI::getDfisanExtAPI();
  if (auto *Call = SVFUtil::dyn_cast<CallInst>(Store)) {
    auto *Callee = Call->getCalledFunction();
    if (Callee == nullptr)  // TODO: indirect call handling
      return AccessOperand();
    auto FnName = Callee->getName().str();
    if (ExtAPI->isExtDefFun(FnName)) {
      auto &ExtFun = ExtAPI->getExtFun(FnName);
      LLVM_DEBUG(llvm::dbgs() << "ExtFun: " << FnName << "\n");
      if (ExtFun.hasSizePos()) {
        Value *SizeVal = Call->getArgOperand(ExtFun.SizePos);
        return AccessOperand(Dst, SizeVal);
      }
      if (ExtFun.Ty == DfisanExtAPI::ExtFunType::EXT_CALLOC) {
        // size = nmem * size_t
        llvm::IRBuilder<> Builder(Call);
        Builder.SetInsertPoint(Call);
        Value *SizeVal = Builder.CreateNUWMul(Call->getOperand(0), Call->getOperand(1));
        LLVM_DEBUG(llvm::dbgs() << "calloc: " << *Call << "\n");
        LLVM_DEBUG(llvm::dbgs() << " - SizeVal: " << *SizeVal << "\n");
        LLVM_DEBUG(llvm::dbgs() << " - Dst: " << *Dst << "\n");
        return AccessOperand(Dst, SizeVal);
      }
    }
  }
  auto &DL = LLVMModuleSet::getLLVMModuleSet()->getModule(0)->getDataLayout();
  unsigned Size = DL.getTypeStoreSize(Dst->getType()->getNonOpaquePointerElementType());
  return AccessOperand(Dst, Size);
}

AccessOperand DefUseSolver::getLoadOperand(NodeID ID) {
  const SVFGNode *Node = Svfg->getSVFGNode(ID);
  assert(SVFUtil::isa<LoadSVFGNode>(Node));
  const LoadSVFGNode *LoadNode = SVFUtil::cast<LoadSVFGNode>(Node);
  Value *Load = (Value *)LoadNode->getValue();
  const PAGNode *SrcNode = LoadNode->getPAGSrcNode();
  Value *Src = (Value *)SrcNode->getValue();
  if (Load == nullptr || Src == nullptr)
    return AccessOperand();
  LLVM_DEBUG(llvm::dbgs() << "Load: " << *Load << "\n");
  LLVM_DEBUG(llvm::dbgs() << " - Src: " << *Src << "\n");
  DfisanExtAPI *ExtAPI = DfisanExtAPI::getDfisanExtAPI();
  if (auto *Call = SVFUtil::dyn_cast<CallInst>(Load)) {
    auto *Callee = Call->getCalledFunction();
    if (Callee == nullptr)  // TODO: indirect call handling
      return AccessOperand();
    auto FnName = Callee->getName().str();
    if (ExtAPI->isExtUseFun(FnName)) {
      auto &ExtFun = ExtAPI->getExtFun(FnName);
      LLVM_DEBUG(llvm::dbgs() << "ExtFun: " << FnName << "\n");
      if (ExtFun.hasSizePos()) {
        Value *SizeVal = Call->getArgOperand(ExtFun.SizePos);
        return AccessOperand(Src, SizeVal);
      }
    }
  }
  auto &DL = LLVMModuleSet::getLLVMModuleSet()->getModule(0)->getDataLayout();
  unsigned Size = DL.getTypeStoreSize(Src->getType()->getNonOpaquePointerElementType());
  return AccessOperand(Src, Size);
}

bool DefUseSolver::isUnsafeOperand(AccessOperand &Operand) const {
  Value *Base = Operand.Operand->stripInBoundsConstantOffsets();
  if (GlobalVariable *GlobVar = SVFUtil::dyn_cast<GlobalVariable>(Base))
    return false;
  if (AllocaInst *Alloca = SVFUtil::dyn_cast<AllocaInst>(Base))
    return false;
  return true;
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
