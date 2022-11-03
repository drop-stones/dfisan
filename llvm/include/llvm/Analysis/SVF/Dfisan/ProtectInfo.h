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

/// Type for access operands used by instrumentation
struct AccessOperand {
  Value *Operand;
  Value *SizeVal;
  unsigned Size;

  AccessOperand(Value *Operand, Value *SizeVal) : Operand(Operand), SizeVal(SizeVal), Size(0) {}
  AccessOperand(Value *Operand, unsigned Size)  : Operand(Operand), SizeVal(nullptr), Size(Size) {}
  AccessOperand() : AccessOperand(nullptr, nullptr) {}
  bool operator< (const AccessOperand &Rhs) const {
    return this->Operand < Rhs.Operand;
  }

  bool empty() const { return Operand == nullptr; }
  bool hasSizeVal() const { return SizeVal != nullptr; }
};
using AccessOperandSet = std::set<AccessOperand>;

class ProtectInfo {
  /// Type for DefInfo and UseInfo used by instrumentation
  struct DefInfo {
    DefID ID;
    AccessOperandSet Operands;
    bool IsInstrumented;

    DefInfo(DefID ID, AccessOperand Operand) : ID(ID), IsInstrumented(false) { addOperand(Operand); }
    DefInfo() : ID(0), IsInstrumented(false) {}
    void addOperand(AccessOperand &Operand) { Operands.insert(Operand); }
    void setDefID(DefID ID) { this->ID = ID; }
  };
  using DefInfoMap = std::unordered_map<llvm::Value *, DefInfo>;
  struct UseInfo {
    DefIDSet DefIDs;
    AccessOperandSet Operands;
    bool IsInstrumented;

    UseInfo(DefID ID, AccessOperand Operand) : IsInstrumented(false) { addDefID(ID); addOperand(Operand); }
    UseInfo() : IsInstrumented(false) {}
    void addOperand(AccessOperand &Operand) { Operands.insert(Operand); }
    void addDefID(DefID ID) { DefIDs.insert(ID); }
  };
  using UseInfoMap = std::unordered_map<llvm::Value *, UseInfo>;
  struct UnsafeInstInfo {
    AccessOperandSet Operands;
    UnsafeInstInfo() {}
    void addOperand(AccessOperand &Operand) { Operands.insert(Operand); }
  };
  using UnsafeInstInfoMap = std::unordered_map<llvm::Instruction *, UnsafeInstInfo>;

public:
  ProtectInfo(ValueSet &Aligned, ValueSet &Unaligned)
    : Aligned(Aligned), Unaligned(Unaligned) {}
  
  bool emptyTarget() { return Aligned.empty() && Unaligned.empty(); }
  bool hasAlignedTarget(llvm::Value *V)   { return Aligned.count(V) != 0; }
  bool hasUnalignedTarget(llvm::Value *V) { return Unaligned.count(V) != 0; }
  bool hasTarget(llvm::Value *V) { return hasAlignedTarget(V) || hasUnalignedTarget(V); }
  bool hasDef(llvm::Value *Def) { return DefToInfo.count(Def) != 0; }
  bool hasUse(llvm::Value *Use) { return UseToInfo.count(Use) != 0; }

  void insertAlignedOnlyDef(llvm::Value *Def)         { AlignedOnlyDefs.insert(Def); }
  void insertUnalignedOnlyDef(llvm::Value *Def)       { UnalignedOnlyDefs.insert(Def); }
  void insertBothOnlyDef(llvm::Value *Def)            { BothOnlyDefs.insert(Def); }
  void insertAlignedOrNoTargetDef(llvm::Value *Def)   { AlignedOrNoTargetDefs.insert(Def); }
  void insertUnalignedOrNoTargetDef(llvm::Value *Def) { UnalignedOrNoTargetDefs.insert(Def); }
  void insertBothOrNoTargetDef(llvm::Value *Def)      { BothOrNoTargetDefs.insert(Def); }

  void insertAlignedOnlyUse(llvm::Value *Use)         { AlignedOnlyUses.insert(Use); }
  void insertUnalignedOnlyUse(llvm::Value *Use)       { UnalignedOnlyUses.insert(Use); }
  void insertBothOnlyUse(llvm::Value *Use)            { BothOnlyUses.insert(Use); }
  void insertAlignedOrNoTargetUse(llvm::Value *Use)   { AlignedOrNoTargetUses.insert(Use); }
  void insertUnalignedOrNoTargetUse(llvm::Value *Use) { UnalignedOrNoTargetUses.insert(Use); }
  void insertBothOrNoTargetUse(llvm::Value *Use)      { BothOrNoTargetUses.insert(Use); }

  void setDefID(llvm::Value *Def, DefID ID) {
    DefToInfo[Def].setDefID(ID);
  }

  void addDefOperand(llvm::Value *Def, AccessOperand &Operand) {
    DefToInfo[Def].addOperand(Operand);
  }

  void addUseOperand(llvm::Value *Use, AccessOperand &Operand) {
    UseToInfo[Use].addOperand(Operand);
  }

  void addUseDef(llvm::Value *Use, DefID ID) {
    UseToInfo[Use].addDefID(ID);
  }

  void addUnsafeOperand(llvm::Instruction *UnsafeInst, AccessOperand &Operand) {
    UnsafeToInfo[UnsafeInst].addOperand(Operand);
  }

  DefID getDefID(llvm::Value *Def) {
    assert(hasDef(Def) && "No Def value");
    return DefToInfo[Def].ID;
  }

  const DefInfo &getDefInfo(llvm::Value *Def) {
    assert(hasDef(Def));
    return DefToInfo[Def];
  }
  const UseInfo &getUseInfo(llvm::Value *Use) {
    assert(hasUse(Use));
    return UseToInfo[Use];
  }

  /// Getter
  const DefInfoMap &getDefToInfo() { return DefToInfo; }
  const UnsafeInstInfoMap &getUnsafeToInfo() { return UnsafeToInfo; }

  const ValueSet &getAlignedOnlyDefs()         { return AlignedOnlyDefs; }
  const ValueSet &getUnalignedOnlyDefs()       { return UnalignedOnlyDefs; }
  const ValueSet &getBothOnlyDefs()            { return BothOnlyDefs; }
  const ValueSet &getAlignedOrNoTargetDefs()   { return AlignedOrNoTargetDefs; }
  const ValueSet &getUnalignedOrNoTargetDefs() { return UnalignedOrNoTargetDefs; }
  const ValueSet &getBothOrNoTargetDefs()      { return BothOrNoTargetDefs; }
  const ValueSet &getAlignedOnlyUses()         { return AlignedOnlyUses; }
  const ValueSet &getUnalignedOnlyUses()       { return UnalignedOnlyUses; }
  const ValueSet &getBothOnlyUses()            { return BothOnlyUses; }
  const ValueSet &getAlignedOrNoTargetUses()   { return AlignedOrNoTargetUses; }
  const ValueSet &getUnalignedOrNoTargetUses() { return UnalignedOrNoTargetUses; }
  const ValueSet &getBothOrNoTargetUses()      { return BothOrNoTargetUses; }

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

    OS << "UnsafeInst:\n";
    for (const auto &Iter : UnsafeToInfo) {
      OS << " - UnsafeInst: " << *Iter.first << "\n";
      for (const auto &Ope : Iter.second.Operands)
        OS << "   - Operand: " << *Ope.Operand << "\n";
    }

    OS << "Defs:\n";
    for (const auto &Iter : DefToInfo) {
      OS << " - Def(" << Iter.second.ID << "): " << *Iter.first << "\n";
      for (const auto &Ope : Iter.second.Operands)
        if (Ope.hasSizeVal())
          OS << "   - SizeVal(" << *Ope.SizeVal << "), Operand: " << *Ope.Operand << "\n";
        else
          OS << "   - Size(" << Ope.Size << "), Operand: " << *Ope.Operand << "\n";
    }
    OS << "Use + DefIDs:\n";
    for (const auto &Iter : UseToInfo) {
      OS << " - Use: " << *Iter.first << "\n";
      OS << "   - DefIDs: { ";
      for (auto DefID : Iter.second.DefIDs)
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

  /// Map from Def and Use to info used by instrumentation
  DefInfoMap DefToInfo;
  UseInfoMap UseToInfo;
  UnsafeInstInfoMap UnsafeToInfo;
};

} // namespace SVF

#endif
