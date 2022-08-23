#ifndef LLVM_ANALYSIS_DG_PASSES_DFIPROTECTINFO_H
#define LLVM_ANALYSIS_DG_PASSES_DFIPROTECTINFO_H

#include <set>
#include "llvm/IR/Value.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/raw_ostream.h"

namespace dg {

using ValueSet = std::set<const llvm::Value *>;

struct DfiProtectInfo {
private:
  bool isProtectTarget(const llvm::Value *V) {
    if (const auto *GlobalVal = llvm::dyn_cast<llvm::GlobalVariable>(V)) {
      if (GlobalVal->isConstant() || GlobalVal->getName() == "llvm.global.annotations")
        return false;
    }
    return true;
  }

public:
  DfiProtectInfo() {}

  bool isSelectiveDfi() {
    return !(Globals.empty() && Locals.empty() && Heaps.empty());
  }

  void insertGlobal(const llvm::Value *G) {
    if (isProtectTarget(G))
      Globals.insert(G);
  }
  void insertLocal(const llvm::Value *L) { Locals.insert(L); }
  void insertHeap(const llvm::Value *H)  { Heaps.insert(H); }
  void insertProtectPtr(const llvm::Value *Ptr) { ProtectPtrs.insert(Ptr); }
  void insertDef(const llvm::Value *D) {
    if (isProtectTarget(D))
      Defs.insert(D);
  }
  void insertUse(const llvm::Value *U) { Uses.insert(U); }

  bool hasValue(const llvm::Value *V) {
    return hasGlobal(V) || hasLocal(V) || hasHeap(V);
  }
  bool hasGlobal(const llvm::Value *G) { return Globals.count(G) != 0; }
  bool hasLocal(const llvm::Value *L)  { return Locals.count(L) != 0; }
  bool hasHeap(const llvm::Value *H)   { return Heaps.count(H) != 0; }
  bool hasProtectPtr(const llvm::Value *Ptr) { return ProtectPtrs.count(Ptr) != 0; }

  void dump(llvm::raw_ostream &OS) {
    OS << "DfiProtectInfo::" << __func__ << "\n";
    OS << "DfiProtectionList:\n";
    OS << " - Globals:\n";
    for (const auto *Global : Globals)
      OS << "   - " << *Global << "\n";
    OS << " - Locals:\n";
    for (const auto *Local : Locals)
      OS << "   - " << *Local << "\n";
    OS << " - Heaps:\n";
    for (const auto *Heap : Heaps)
      OS << "   - " << *Heap << "\n";
    OS << " - ProtectPtrs:\n";
    for (const auto *Ptr : ProtectPtrs)
      OS << "   - " << *Ptr << "\n";
    
    OS << "Def instructions:\n";
    for (const auto *Def : Defs)
      OS << " - " << *Def << "\n";
    OS << "Use instructions:\n";
    for (const auto *Use : Uses)
      OS << " - " << *Use << "\n";
  }

  /// Protection Targets
  ValueSet Globals;
  ValueSet Locals;
  ValueSet Heaps;

  /// Pointers which points-to heap objects.
  ValueSet ProtectPtrs;

  ValueSet Defs;
  ValueSet Uses;
};

} // namespace dg

#endif
