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

  void insertDef(llvm::Value *Def, DefID ID) {
    DefToInfo[Def] = DefInfo(ID);
  }

  void addUseDef(llvm::Value *Use, DefID ID) {
    UseToDefIDs[Use].insert(ID);
  }

  void dump(llvm::raw_ostream &OS) {
    OS << "ProtectInfo::" << __func__ << "\n";
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

  DefInfoMap DefToInfo;
  UseDefIDMap UseToDefIDs;
};

} // namespace SVF

#endif
