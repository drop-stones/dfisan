#ifndef LLVM_ANALYSIS_DG_PASSES_DFIPROTECTINFO_H
#define LLVM_ANALYSIS_DG_PASSES_DFIPROTECTINFO_H

#include <unordered_set>
#include <unordered_map>

#include "llvm/ADT/SparseBitVector.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/raw_ostream.h"

namespace dg {

using ValueSet = std::unordered_set<llvm::Value *>;
using InstSet = std::unordered_set<llvm::Instruction *>;
using UseDefMap = std::unordered_map<llvm::Instruction *, ValueSet>;

using DefID = uint16_t;
struct DefInfo {
  DefID ID;
  bool IsInstrumented;

  DefInfo(DefID ID) : ID(ID), IsInstrumented(false) {}
  DefInfo() : DefInfo(0) {}
};
using DefInfoMap = std::unordered_map<llvm::Value *, DefInfo>;

/// Class to store protection targets and its defs and uses.
struct DfiProtectInfo {
public:
  DfiProtectInfo(ValueSet &Aligned, ValueSet &Unaligned)
    : AlignedTargets(Aligned), UnalignedTargets(Unaligned) {}

  bool emptyTarget() { return AlignedTargets.empty() && UnalignedTargets.empty(); }
  bool hasAlignedTarget(llvm::Value *V)   { return AlignedTargets.count(V) != 0; }
  bool hasUnalignedTarget(llvm::Value *V) { return UnalignedTargets.count(V) != 0; }
  bool hasTarget(llvm::Value *V) { return hasAlignedTarget(V) || hasUnalignedTarget(V); }

  void insertAlignedOnlyDef(llvm::Value *Def)         { AlignedOnlyDefs.insert(Def); assignDefID(Def); }
  void insertUnalignedOnlyDef(llvm::Value *Def)       { UnalignedOnlyDefs.insert(Def); assignDefID(Def); }
  void insertBothOnlyDef(llvm::Value *Def)            { BothOnlyDefs.insert(Def); assignDefID(Def); }
  void insertAlignedOrNoTargetDef(llvm::Value *Def)   { AlignedOrNoTargetDefs.insert(Def); assignDefID(Def); }
  void insertUnalignedOrNoTargetDef(llvm::Value *Def) { UnalignedOrNoTargetDefs.insert(Def); assignDefID(Def); }
  void insertBothOrNoTargetDef(llvm::Value *Def)      { BothOrNoTargetDefs.insert(Def); assignDefID(Def); }

  void insertAlignedOnlyUse(llvm::Instruction *Use)         { Uses.insert(Use); AlignedOnlyUses.insert(Use); }
  void insertUnalignedOnlyUse(llvm::Instruction *Use)       { Uses.insert(Use); UnalignedOnlyUses.insert(Use); }
  void insertBothOnlyUse(llvm::Instruction *Use)            { Uses.insert(Use); BothOnlyUses.insert(Use); }
  void insertAlignedOrNoTargetUse(llvm::Instruction *Use)   { Uses.insert(Use); AlignedOrNoTargetUses.insert(Use); }
  void insertUnalignedOrNoTargetUse(llvm::Instruction *Use) { Uses.insert(Use); UnalignedOrNoTargetUses.insert(Use); }
  void insertBothOrNoTargetUse(llvm::Instruction *Use)      { Uses.insert(Use); BothOrNoTargetUses.insert(Use); }

  void insertUseDef(llvm::Instruction *Use, llvm::Value *Def) {
    if (!hasDef(Def) || !hasUse(Use)) // the Def or Use is not an access of protection targets
      return;
    UseDef[Use].insert(Def); 
  }

  bool hasDefID(llvm::Value *Def) {
    return DefToInfo.count(Def) != 0;
  }
  DefID getDefID(llvm::Value *Def) {
    assert(hasDefID(Def) && "No Def value");
    return DefToInfo[Def].ID;
  }

  bool hasDef(llvm::Value *Def) {
    return hasDefID(Def);
  }
  bool hasUse(llvm::Instruction *Use) {
    return Uses.count(Use) != 0;
  }

  void dump(llvm::raw_ostream &OS) {
    OS << "DfiProtectInfo::" << __func__ << "\n";
    OS << "Aligned Targets:\n";
    for (auto *Aligned : AlignedTargets)
      OS << " - " << *Aligned << "\n";
    OS << "Unaligned Targets:\n";
    for (auto *Unaligned : UnalignedTargets)
      OS << " - " << *Unaligned << "\n";
    
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

    OS << "UseDef\n";
    for (const auto &Iter : UseDef) {
      OS << "Use: " << *Iter.first << "\n";
      std::set<DefID> IDs;
      for (auto *Def : Iter.second)
        IDs.insert(getDefID(Def));
      OS << " - DefID: { ";
      for (auto ID : IDs)
        OS << ID << ", ";
      OS << "}\n";
    }
    OS.flush();
  }

  /// Protection Targets
  ValueSet &AlignedTargets;
  ValueSet &UnalignedTargets;

  /// Defs by kind
  ValueSet AlignedOnlyDefs;
  ValueSet UnalignedOnlyDefs;
  ValueSet BothOnlyDefs;
  ValueSet AlignedOrNoTargetDefs;
  ValueSet UnalignedOrNoTargetDefs;
  ValueSet BothOrNoTargetDefs;

  /// Uses by kind
  InstSet AlignedOnlyUses;
  InstSet UnalignedOnlyUses;
  InstSet BothOnlyUses;
  InstSet AlignedOrNoTargetUses;
  InstSet UnalignedOrNoTargetUses;
  InstSet BothOrNoTargetUses;

  DefInfoMap DefToInfo;
  InstSet Uses;
  UseDefMap UseDef;

  struct UseSet {
    llvm::SparseBitVector<> Uses;
    ValueSet Defs;
    UseSet(llvm::SparseBitVector<> Uses, ValueSet Defs) : Uses(Uses), Defs(Defs) {}
  };

  // Rename optimization:
  //   Rename DefID-a to DefID-b if the Def-a instruction and the Def-b instruction
  //   has the same use sets.
  void renameDefIDs() {
    using DefToUse = std::unordered_map<llvm::Value *, llvm::SparseBitVector<>>;
    using UseToDef = std::vector<UseSet>;
    using UseVec = std::vector<llvm::Instruction *>;
    DefToUse DefUse;
    UseToDef RenameVec;
    UseVec Uses;
    // Construct Def-Use map from Use-Def.
    for (auto &Iter : UseDef) {
      auto *Use = Iter.first;
      auto &Defs = Iter.second;
      Uses.push_back(Use);
      unsigned int UseID = Uses.size() - 1;
      for (auto *Def : Defs)
        DefUse[Def].test_and_set(UseID);
    }
    // Construct a map of UseSet to Defs.
    for (auto &Iter : DefUse) {
      auto *Def = Iter.first;
      auto &Uses = Iter.second;
      UseToDef::iterator Ptr;
      for (Ptr = RenameVec.begin(); Ptr != RenameVec.end(); Ptr++) {
        if (Ptr->Uses == Uses) {
          Ptr->Defs.insert(Def);
          break;
        }
      }
      if (Ptr == RenameVec.end()) { // new element of RenameVec
        RenameVec.emplace_back(Uses, ValueSet{Def});
      }
    }
    // Rename DefIDs
    for (auto &UseSet : RenameVec) {
      assert(!UseSet.Defs.empty() && "UseSet.Defs is empty");
      auto *Representative = *UseSet.Defs.begin();
      DefID ID = getDefID(Representative);
      for (auto *Def : UseSet.Defs) {
        DefToInfo[Def].ID = ID;
      }
    }
    // Print for debug
    // for (auto &UseSet : RenameVec) {
    //   llvm::errs() << "Same DefID set:\n";
    //   for (auto *Def : UseSet.Defs)
    //     llvm::errs() << " - " << *Def << "\n";
    // }
  }

private:
  const DefID InitID = 1;
  DefID CurrID = InitID;

  void assignDefID(llvm::Value *Def) {
    if (hasDefID(Def))  // Already assigned
      return;
    
    DefToInfo.emplace(Def, CurrID);

    // Calculate the next DefID.
    if (CurrID == USHRT_MAX) {
      llvm::errs() << "DefID overflow!\n";
      CurrID = InitID;
    } else {
      CurrID++;
    }
  }
};

} // namespace dg

#endif
