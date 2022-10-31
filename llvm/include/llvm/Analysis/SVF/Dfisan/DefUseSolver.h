#ifndef DFISAN_DEFUSE_SOLVER_H_
#define DFISAN_DEFUSE_SOLVER_H_

#include "MTA/MTA.h"
#include "Dfisan/DfisanSVFG.h"
#include "Dfisan/ProtectInfo.h"
#include "Util/WorkList.h"

namespace SVF {

struct DefUseKind;

class DefUseSolver {
public:
  /// Define worklist
  using WorkList = FIFOWorkList<NodeID>;
  /// Type for data-flow facts (NodeID)
  using DefIDVec = llvm::SparseBitVector<>;
  using UseIDVec = llvm::SparseBitVector<>;
  using NodeToDefIDMap = std::unordered_map<NodeID, DefIDVec>;
  using NodeToUseIDMap = std::unordered_map<NodeID, UseIDVec>; 
  using UseDefIDMap = NodeToDefIDMap;
  using DefUseIDMap = NodeToUseIDMap;
  using ValueToNodeIDMap = std::unordered_map<llvm::Value *, NodeID>;

  /// Class for def-use map of NodeID.
  struct DefUseIDInfo {
    DefUseIDMap DefUseID;
    UseDefIDMap UseDefID;
    DefUseIDMap DataRaceDefUseID;
    UseDefIDMap DataRaceUseDefID;
    ValueToNodeIDMap ValToUniqueID;

    DefUseIDInfo() {}
    void insertDefUseID(NodeID Def, NodeID Use) {
      DefUseID[Def].set(Use);
      UseDefID[Use].set(Def);
    }
    void insertDataRaceDefUseID(NodeID Def, NodeID Use) {
      DataRaceDefUseID[Def].set(Use);
      DataRaceUseDefID[Use].set(Def);
    }
    NodeID getUniqueID(Value *Val, NodeID DefID) {
      if (ValToUniqueID.count(Val) == 0)  // New value
        ValToUniqueID[Val] = DefID;
      else
        DefID = ValToUniqueID[Val];
      return DefID;
    }
  };

  /// Class to store equivalent def set.
  struct EquivalentDefSet {
    DefIDVec Defs;  // Value
    UseIDVec Uses;  // Key
    EquivalentDefSet(NodeID DefID, UseIDVec Uses) : Defs(), Uses(Uses) { Defs.set(DefID); }
  };

  /// Constructor
  DefUseSolver(SVFG *Svfg, MHP *Mhp, LockAnalysis *LockAna, ProtectInfo *ProtInfo)
    : Svfg(Svfg), Mhp(Mhp), LockAna(LockAna), Pta(Svfg->getPTA()), Pag(Svfg->getPAG()), ProtInfo(ProtInfo) {}

  /// Start solving
  void solve();

  /// Return SVFG
  inline const SVFG *getSVFG() const { return Svfg; }

protected:
  /// Worklist operations
  //@{
  inline bool pushIntoWorklist(NodeID ID) {
    return Worklist.push(ID);
  }
  inline NodeID popFromWorklist() { 
    return Worklist.pop();
  }
  inline bool isInWorklist(NodeID ID) {
    return Worklist.find(ID);
  }
  inline bool isWorklistEmpty() {
    return Worklist.empty();
  }
  //@}

private:
  SVFG *Svfg;
  MHP *Mhp;
  LockAnalysis *LockAna;
  PointerAnalysis *Pta;
  SVFIR *Pag;
  ProtectInfo *ProtInfo;
  WorkList Worklist;

  /// Get Value * from NodeID
  Value *getValue(NodeID ID);

  /// Return true if two Values are data race
  bool isDataRace(Value *V1, Value *V2);

  /// Add DefUse to DefUseIDInfo.
  void addDefUse(DefUseIDInfo &DefUse, NodeID Def, NodeID Use);

  /// DefID assignment
  const DefID InitID = 1;
  DefID CurrID = InitID;

  /// Get next DefID
  DefID getNextID() {
    DefID Ret = CurrID;
    if (CurrID == USHRT_MAX) {
      llvm::errs() << "DefID overflow\n";
      CurrID = InitID;
    } else {
      CurrID++;
    }
    return Ret;
  }

  /// Register renaming optimized UseDef to ProtectInfo
  void registerUseDef(std::vector<EquivalentDefSet> &EquivalentDefs);

  /// Register UseDef to ProtectInfo
  void registerUseDef(DefUseIDInfo &DefUse);

  /// Calculate equivalent defs
  void calcEquivalentDefSet(DefUseIDInfo &DefUse, std::vector<EquivalentDefSet> &EquivalentDefs);

  /// Access target analysis
  bool isTargetStore(NodeID ID);
  bool isTargetLoad(NodeID ID);
  bool containTarget(const PointsTo &PtsSet);
  void getValueSetFromPointsTo(ValueSet &Values, const PointsTo &PtsSet);
  const PointsTo &collectStoreTarget(NodeID ID);
  const PointsTo &collectLoadTarget(NodeID ID);

  /// DefUseKind analysis
  DefUseKind analyzeDefKind(NodeID ID);
  DefUseKind analyzeUseKind(NodeID ID);
  void setDefUseKind(DefUseKind &Kind, const PointsTo &PtsSet);
};

struct DefUseKind {
  DefUseKind() : IsDef(false), IsUse(false), IsAligned(false), IsUnaligned(false), IsNoTarget(false) {}

  /// Def kind
  bool isAlignedOnlyDef()         { return IsDef &&  IsAligned && !IsUnaligned && !IsNoTarget; }
  bool isUnalignedOnlyDef()       { return IsDef && !IsAligned &&  IsUnaligned && !IsNoTarget; }
  bool isBothOnlyDef()            { return IsDef &&  IsAligned &&  IsUnaligned && !IsNoTarget; }
  bool isAlignedOrNoTargetDef()   { return IsDef &&  IsAligned && !IsUnaligned &&  IsNoTarget; }
  bool isUnalignedOrNoTargetDef() { return IsDef && !IsAligned &&  IsUnaligned &&  IsNoTarget; }
  bool isBothOrNoTargetDef()      { return IsDef &&  IsAligned &&  IsUnaligned &&  IsNoTarget; }

  /// Use kind
  bool isAlignedOnlyUse()         { return IsUse &&  IsAligned && !IsUnaligned && !IsNoTarget; }
  bool isUnalignedOnlyUse()       { return IsUse && !IsAligned &&  IsUnaligned && !IsNoTarget; }
  bool isBothOnlyUse()            { return IsUse &&  IsAligned &&  IsUnaligned && !IsNoTarget; }
  bool isAlignedOrNoTargetUse()   { return IsUse &&  IsAligned && !IsUnaligned &&  IsNoTarget; }
  bool isUnalignedOrNoTargetUse() { return IsUse && !IsAligned &&  IsUnaligned &&  IsNoTarget; }
  bool isBothOrNoTargetUse()      { return IsUse &&  IsAligned &&  IsUnaligned &&  IsNoTarget; }

  // Unused funcs
  bool isNoKind()   { return !IsAligned && !IsUnaligned && !IsNoTarget; }
  bool isNoTarget() { return !IsAligned && !IsUnaligned && IsNoTarget; }

  bool IsDef;
  bool IsUse;
  bool IsAligned;
  bool IsUnaligned;
  bool IsNoTarget;
};

} // namespace SVF

#endif
