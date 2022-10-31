#ifndef DFISAN_PROTECT_INFO_H_
#define DFISAN_PROTECT_INFO_H_

#include <unordered_set>
#include <unordered_map>

#include "Util/BasicTypes.h"

namespace SVF {

/// Type for llvm::Value
using ValueSet = std::unordered_set<llvm::Value *>;

/// Type for DefID used by instrumentation
using DefID = uint16_t;
using DefIDSet = std::unordered_set<DefID>;

class ProtectInfo {
  /// Type for llvm::Value
  using UseDefMap = std::unordered_map<llvm::Value *, ValueSet>;
  using UseDefIDMap = std::unordered_map<llvm::Value *, DefIDSet>;
  /// Type for DefID used by instrumentation
  struct DefInfo {
    DefID ID;
    bool IsInstrumented;

    DefInfo(DefID ID) : ID(ID), IsInstrumented(false) {}
    DefInfo() : DefInfo(0) {}
  };
  using DefInfoMap = std::unordered_map<llvm::Value *, DefInfo>;

public:
  ProtectInfo(ValueSet &Aligned, ValueSet &Unaligned)
    : Aligned(Aligned), Unaligned(Unaligned) {}
  
  bool emptyTarget() { return Aligned.empty() && Unaligned.empty(); }
  bool hasAlignedTarget(llvm::Value *V)   { return Aligned.count(V) != 0; }
  bool hasUnalignedTarget(llvm::Value *V) { return Unaligned.count(V) != 0; }
  bool hasTarget(llvm::Value *V) { return hasAlignedTarget(V) || hasUnalignedTarget(V); }
  bool hasDef(llvm::Value *Def) { return DefToInfo.count(Def) != 0; }
  bool hasUse(llvm::Value *Use) { return UseToDefIDs.count(Use) != 0; }

  void insertAlignedOnlyDef(llvm::Value *Def, DefID ID)         { AlignedOnlyDefs.insert(Def); setDefID(Def, ID); }
  void insertUnalignedOnlyDef(llvm::Value *Def, DefID ID)       { UnalignedOnlyDefs.insert(Def); setDefID(Def, ID); }
  void insertBothOnlyDef(llvm::Value *Def, DefID ID)            { BothOnlyDefs.insert(Def); setDefID(Def, ID); }
  void insertAlignedOrNoTargetDef(llvm::Value *Def, DefID ID)   { AlignedOrNoTargetDefs.insert(Def); setDefID(Def, ID); }
  void insertUnalignedOrNoTargetDef(llvm::Value *Def, DefID ID) { UnalignedOrNoTargetDefs.insert(Def); setDefID(Def, ID); }
  void insertBothOrNoTargetDef(llvm::Value *Def, DefID ID)      { BothOrNoTargetDefs.insert(Def); setDefID(Def, ID); }

  void insertAlignedOnlyUse(llvm::Value *Use)         { AlignedOnlyUses.insert(Use); }
  void insertUnalignedOnlyUse(llvm::Value *Use)       { UnalignedOnlyUses.insert(Use); }
  void insertBothOnlyUse(llvm::Value *Use)            { BothOnlyUses.insert(Use); }
  void insertAlignedOrNoTargetUse(llvm::Value *Use)   { AlignedOrNoTargetUses.insert(Use); }
  void insertUnalignedOrNoTargetUse(llvm::Value *Use) { UnalignedOrNoTargetUses.insert(Use); }
  void insertBothOrNoTargetUse(llvm::Value *Use)      { BothOrNoTargetUses.insert(Use); }

  void setDefID(llvm::Value *Def, DefID ID) {
    DefToInfo[Def] = DefInfo(ID);
  }

  void addUseDef(llvm::Value *Use, DefID ID) {
    UseToDefIDs[Use].insert(ID);
  }

  DefID getDefID(llvm::Value *Def) {
    assert(hasDef(Def) && "No Def value");
    return DefToInfo[Def].ID;
  }
  const DefIDSet &getDefIDsFromUse(llvm::Value *Use) {
    assert(hasUse(Use));
    return UseToDefIDs[Use];
  }
  const DefInfoMap &getDefToInfo() { return DefToInfo; }

  void dump(llvm::raw_ostream &OS) {
    OS << "ProtectInfo::" << __func__ << "\n";

    OS << "Aligned Targets:\n";
    for (auto *AlignedTgt : Aligned)
      OS << " - " << *AlignedTgt << "\n";
    OS << "Unaligned Targets:\n";
    for (auto *UnalignedTgt : Unaligned)
      OS << " - " << *UnalignedTgt << "\n";
    
    OS << "AlignedOnly Def instructions:\n";
    for (auto *Def : AlignedOnlyDefs)
      OS << " - DefID[" << DefToInfo[Def].ID << "] " << *Def << "\n";
    OS << "UnalignedOnly Def instructions:\n";
    for (auto *Def : UnalignedOnlyDefs)
      OS << " - DefID[" << DefToInfo[Def].ID << "] " << *Def << "\n";
    OS << "BothOnly Def instructions:\n";
    for (auto *Def : BothOnlyDefs)
      OS << " - DefID[" << DefToInfo[Def].ID << "] " << *Def << "\n";
    OS << "AlignedOrNoTarget Def instructions:\n";
    for (auto *Def : AlignedOrNoTargetDefs)
      OS << " - DefID[" << DefToInfo[Def].ID << "] " << *Def << "\n";
    OS << "UnalignedOrNoTarget Def instructions:\n";
    for (auto *Def : UnalignedOrNoTargetDefs)
      OS << " - DefID[" << DefToInfo[Def].ID << "] " << *Def << "\n";
    OS << "BothOrNoTarget Def instructions:\n";
    for (auto *Def : BothOrNoTargetDefs)
      OS << " - DefID[" << DefToInfo[Def].ID << "] " << *Def << "\n";

    OS << "AlignedOnly Use instructions:\n";
    for (auto *Use : AlignedOnlyUses)
      OS << " - " << *Use << "\n";
    OS << "UnalignedOnly Use instructions:\n";
    for (auto *Use : UnalignedOnlyUses)
      OS << " - " << *Use << "\n";
    OS << "BothOnly Use instructions:\n";
    for (auto *Use : BothOnlyUses)
      OS << " - " << *Use << "\n";
    OS << "AlignedOrNoTarget Use instructions:\n";
    for (auto *Use : AlignedOrNoTargetUses)
      OS << " - " << *Use << "\n";
    OS << "UnalignedOrNoTarget Use instructions:\n";
    for (auto *Use : UnalignedOrNoTargetUses)
      OS << " - " << *Use << "\n";
    OS << "BothOrNoTarget Use instructions:\n";
    for (auto *Use : BothOrNoTargetUses)
      OS << " - " << *Use << "\n";

    OS << "Defs:\n";
    for (const auto &Iter : DefToInfo)
      OS << " - Def(" << Iter.second.ID << "): " << *Iter.first << "\n";
    OS << "Use + DefIDs:\n";
    for (const auto &Iter : UseToDefIDs) {
      OS << " - Use: " << *Iter.first << "\n";
      OS << "   - DefIDs: { ";
      for (auto DefID : Iter.second)
        OS << DefID << ", ";
      OS << "}\n";
    }
  }

private:
  /// Protection Targets
  ValueSet &Aligned;
  ValueSet &Unaligned;

  /// Defs by DefUseKind
  ValueSet AlignedOnlyDefs;
  ValueSet UnalignedOnlyDefs;
  ValueSet BothOnlyDefs;
  ValueSet AlignedOrNoTargetDefs;
  ValueSet UnalignedOrNoTargetDefs;
  ValueSet BothOrNoTargetDefs;

  /// Uses by DefUseKind
  ValueSet AlignedOnlyUses;
  ValueSet UnalignedOnlyUses;
  ValueSet BothOnlyUses;
  ValueSet AlignedOrNoTargetUses;
  ValueSet UnalignedOrNoTargetUses;
  ValueSet BothOrNoTargetUses;

  DefInfoMap DefToInfo;
  UseDefIDMap UseToDefIDs;
};

} // namespace SVF

#endif
