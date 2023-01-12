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
  using DefDefIDMap = NodeToDefIDMap;
  using ValueToNodeIDMap = std::unordered_map<llvm::Value *, NodeID>;

  /// Class to allocate unique NodeID to llvm::Value *.
  ValueToNodeIDMap ValToUniqueID;
  ValueToNodeIDMap GlobInitToUniqueID;

  bool isGlobalInit(NodeID ID) {
    Value *Ope = getStoreOperand(ID).Operand;
    const SVFGNode *Node = Svfg->getSVFGNode(ID);
    return SVFUtil::isa<GlobalICFGNode>(Node->getICFGNode()) &&
           SVFUtil::isa<GlobalVariable>(Ope);
  }
  NodeID getUniqueID(Value *Val, NodeID ID) {
    NodeID UniqueID;
    Value *Ope = getStoreOperand(ID).Operand;
    if (isGlobalInit(ID)) {
      // Global init
      if (GlobInitToUniqueID.count(Ope) == 0)
        UniqueID = GlobInitToUniqueID[Ope] = ID;
      else
        UniqueID = GlobInitToUniqueID[Ope];
    } else {
      if (ValToUniqueID.count(Val) == 0)  // New value
        UniqueID = ValToUniqueID[Val] = ID;
      else
        UniqueID = ValToUniqueID[Val];
    }
    return UniqueID;
  }

  /// Class for def-use map of NodeID.
  struct DefUseIDInfo {
    // No-race ID maps
    DefUseIDMap DefUseID;
    UseDefIDMap UseDefID;
    DefDefIDMap RaceCheckToDefID;   // Map from checked IDs(=Def) to may-race ID(=Use)
    DefDefIDMap RaceDefToCheckID;   // Map from may-race ID(=Use) to checked IDs(=Def)
    DefIDVec UnusedDefIDs;
    DefIDVec AllDefIDs;
    // Race ID maps
    DefUseIDMap DataRaceDefUseID;
    UseDefIDMap DataRaceUseDefID;
    DefDefIDMap WriteWriteRaceID;

    DefUseIDInfo() {}
    void insertDefUseID(NodeID Def, NodeID Use) {
      DefUseID[Def].set(Use);
      UseDefID[Use].set(Def);
      AllDefIDs.set(Def);
    }
    void insertDataRaceDefUseID(NodeID Def, NodeID Use) {
      DataRaceDefUseID[Def].set(Use);
      DataRaceUseDefID[Use].set(Def);
    }
    void insertWriteWriteRaceID(NodeID Def1, NodeID Def2) {
      WriteWriteRaceID[Def1].set(Def2);   AllDefIDs.set(Def1);
      WriteWriteRaceID[Def2].set(Def1);   AllDefIDs.set(Def2);
    }
    void insertNoWriteWriteRaceID(NodeID MayRaceDef, NodeID NoRaceDef) {
      RaceDefToCheckID[MayRaceDef].set(NoRaceDef);  AllDefIDs.set(MayRaceDef);
      RaceCheckToDefID[NoRaceDef].set(MayRaceDef);  AllDefIDs.set(NoRaceDef);
    }
    void insertUnusedDefID(NodeID Def) {
      UnusedDefIDs.set(Def);
    }

    bool hasDef(NodeID Def) {
      return DefUseID.count(Def) != 0 || UnusedDefIDs.test(Def) || RaceCheckToDefID.count(Def) != 0;
    }
    bool isRaceDef(NodeID Def) {
      return DataRaceDefUseID.count(Def) != 0 || WriteWriteRaceID.count(Def) != 0;
    }

    // Get IDs which use the Def (Use or write-write race) 
    UseIDVec getCheckedIDs(NodeID Def) {
      return (RaceCheckToDefID.count(Def) != 0) ? DefUseID[Def] | RaceCheckToDefID[Def]
                                                : DefUseID[Def];
    }
  };

  /// Class to store equivalent def set.
  struct EquivalentDefSet {
    DefIDVec Defs;  // Value
    UseIDVec Uses;  // Key
    EquivalentDefSet(NodeID DefID,  UseIDVec Uses) : Defs(), Uses(Uses) { Defs.set(DefID); }
    EquivalentDefSet(DefIDVec Defs, UseIDVec Uses) : Defs(Defs), Uses(Uses) {}
  };

  /// Constructor
  DefUseSolver(SVFG *Svfg, MHP *Mhp, LockAnalysis *LockAna, ProtectInfo *ProtInfo)
    : Svfg(Svfg), Mhp(Mhp), LockAna(LockAna), Pta(Svfg->getPTA()), Pag(Svfg->getPAG()), ProtInfo(ProtInfo) {}

  /// Start solving
  void solve();

  /// Collect unsafe instructions
  void collectUnsafeInst();

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
  std::unordered_map<Value *, Value *> ExtDefCallToSizeVal;   // Cache for ExtDefCall(e.g., calloc, strcpy) to size value

  /// Get Value * from NodeID
  Value *getValue(NodeID ID);

  /// Expand Field-insensitive objects
  void getExpandPointsTo(Value *V, PointsTo &ExpandedPts);

  /// Return true if two Values are alias
  AliasResult isAlias(Value *V1, Value *V2, bool IsFieldInsensitive);

  /// Return true if two Values are data race
  bool isDataRace(Value *V1, Value *V2);
  bool isDataRace(NodeID ID1, NodeID ID2);

  /// Add DefUse to DefUseIDInfo.
  void addDefUse(DefUseIDInfo &DefUse, NodeID Def, NodeID Use);
  void addWriteWriteRace(DefUseIDInfo &DefUse, NodeID Def1, NodeID Def2);
  void addUnusedDef(DefUseIDInfo &DefUse, NodeID Def);

  /// Add Def and Use operands
  void addDefOperand(NodeID ID);
  void addUseOperand(NodeID ID);

  /// DefID assignment
  const DefID InitID = 1;
  DefID NextID = InitID;

  /// Get next DefID
  DefID getNextID() {
    DefID Ret = NextID;
    if (NextID == USHRT_MAX) {
      llvm::errs() << "DefID overflow\n";
      NextID = InitID;
    } else {
      NextID++;
    }
    return Ret;
  }

  /// Register renaming optimized DefUse to ProtectInfo
  void registerDefUse(std::vector<EquivalentDefSet> &EquivalentDefs);

  /// Register no optimized DefUse to ProtectInfo
  void registerDefUse(DefUseIDInfo &DefUse);

  /// Calculate equivalent defs
  void calcEquivalentDefSet(DefUseIDInfo &DefUse, std::vector<EquivalentDefSet> &EquivalentDefs);

  /// Access target analysis: find or check objects allocated by allocs or mallocs
  bool isTargetStore(NodeID ID);
  bool isTargetLoad(NodeID ID);
  bool isTargetWriteWriteRace(NodeID ID, const llvm::SparseBitVector<> &Defs);
  bool containTarget(const PointsTo &PtsSet);
  void getValueSetFromPointsTo(ValueSet &Values, const PointsTo &PtsSet);
  const PointsTo &collectStoreTarget(NodeID ID);
  const PointsTo &collectLoadTarget(NodeID ID);

  /// Access operand analysis: find operands accessed by store or load
  AccessOperand getStoreOperand(NodeID ID);
  AccessOperand getLoadOperand (NodeID ID);
  bool isUnsafeOperand(AccessOperand &Operand) const;

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
