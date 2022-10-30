#ifndef DFISAN_DEFUSE_SOLVER_H_
#define DFISAN_DEFUSE_SOLVER_H_

#include "MTA/MTA.h"
#include "Dfisan/DfisanSVFG.h"
#include "Dfisan/ProtectInfo.h"
#include "Util/WorkList.h"

namespace SVF {

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
    : Svfg(Svfg), Mhp(Mhp), LockAna(LockAna), ProtInfo(ProtInfo) {}

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
};

} // namespace SVF

#endif
