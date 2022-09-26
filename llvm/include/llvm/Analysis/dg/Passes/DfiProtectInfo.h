#ifndef LLVM_ANALYSIS_DG_PASSES_DFIPROTECTINFO_H
#define LLVM_ANALYSIS_DG_PASSES_DFIPROTECTINFO_H

#include <unordered_set>
#include <unordered_map>

#include "llvm/IR/Value.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/raw_ostream.h"

namespace dg {

using ValueSet = std::unordered_set<llvm::Value *>;
using InstSet = std::unordered_set<llvm::Instruction *>;

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

  bool hasAlignedTarget(llvm::Value *V)   { return AlignedTargets.count(V) != 0; }
  bool hasUnalignedTarget(llvm::Value *V) { return UnalignedTargets.count(V) != 0; }
  bool hasTarget(llvm::Value *V) { return hasAlignedTarget(V) || hasUnalignedTarget(V); }
  bool hasDef(llvm::Value *Def) { return Defs.count(Def) != 0; }
  bool hasUse(llvm::Instruction *Use) { return Uses.count(Use) != 0; }

  void insertDef(llvm::Value *Def) { Defs.insert(Def); assignDefID(Def); }
  void insertUse(llvm::Instruction *Use) { Uses.insert(Use); }

  bool hasDefID(llvm::Value *Def) {
    return DefToInfo.count(Def) != 0;
  }
  DefID getDefID(llvm::Value *Def) {
    assert(hasDefID(Def) && "No Def value");
    return DefToInfo[Def].ID;
  }

  void dump(llvm::raw_ostream &OS) {
    OS << "DfiProtectInfo::" << __func__ << "\n";
    OS << "Aligned Targets:\n";
    for (auto *Aligned : AlignedTargets)
      OS << " - " << *Aligned << "\n";
    OS << "Unaligned Targets:\n";
    for (auto *Unaligned : UnalignedTargets)
      OS << " - " << *Unaligned << "\n";
    
    OS << "Def instructions:\n";
    for (auto *Def : Defs)
      OS << " - DefID[" << DefToInfo[Def].ID << "] " << *Def << "\n";
    OS << "Use instructions:\n";
    for (auto *Use : Uses)
      OS << " - " << *Use << "\n";
  }

  /// Protection Targets
  ValueSet &AlignedTargets;
  ValueSet &UnalignedTargets;

  /// Def and Use of targets
  ValueSet Defs;
  InstSet Uses;

  DefInfoMap DefToInfo;

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
