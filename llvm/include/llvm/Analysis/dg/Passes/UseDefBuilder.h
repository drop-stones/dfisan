
#ifndef LLVM_ANALYSIS_DG_PASSES_USEDEFBUILDER_H
#define LLVM_ANALYSIS_DG_PASSES_USEDEFBUILDER_H

#include "dg/llvm/LLVMDependenceGraph.h"
#include "dg/llvm/LLVMDependenceGraphBuilder.h"
#include "dg/Passes/DfiProtectInfo.h"
#include "dg/Passes/UseDefIterator.h"
#include "dg/Passes/DfiLLVMDependenceGraphBuilder.h"

namespace dg {

using DefID = uint16_t;
struct DefInfo {
  DefID ID;
  bool IsInstrumented;

  DefInfo(DefID ID) : ID(ID), IsInstrumented(false) {}
  DefInfo() : DefInfo(0) {}
};
using DefInfoMap = std::unordered_map<llvm::Value *, DefInfo>;

class UseDefBuilder {
  std::unique_ptr<llvmdg::DfiLLVMDependenceGraphBuilder> DgBuilder{nullptr};
public:
  UseDefBuilder(llvm::Module *M)
    : UseDefBuilder(M, {}) {}
  UseDefBuilder(llvm::Module *M, const llvmdg::LLVMDependenceGraphOptions &Opts)
    : DgBuilder(new llvmdg::DfiLLVMDependenceGraphBuilder(ProtectInfo, M, Opts)), M(M) {}
  
  LLVMDependenceGraph *getDG() { return DG.get(); }
  LLVMDataDependenceAnalysis *getDDA() { return DG->getDDA(); }

  LLVMDependenceGraph *buildDG() {
    findDfiProtectTargets();
    DG = DgBuilder->build();
    return DG.get();
  }

  const std::vector<dda::RWNode *> &getGlobals() {
    return getDDA()->getGlobals();
  }

  bool isDef(llvm::Value *Def);
  bool isUse(llvm::Value *Use);

  void findDfiProtectTargets();
  bool isSelectiveDfi() { return ProtectInfo.isSelectiveDfi(); }
  DfiProtectInfo &getProtectInfo() { return ProtectInfo; }

  void assignDefIDs();
  bool hasDefID(llvm::Value *Key);
  DefID getDefID(llvm::Value *Key);

  void printUseDef(llvm::raw_ostream &OS);
  void printDefInfoMap(llvm::raw_ostream &OS);
  void printProtectInfo(llvm::raw_ostream &OS);
  void dump(llvm::raw_ostream &OS);

private:
  std::unique_ptr<LLVMDependenceGraph> DG{nullptr};
  DefInfoMap DefToInfo;
  DfiProtectInfo ProtectInfo;
  const std::string DfiProtectAnn = "dfi_protection";
  llvm::Module *M;

  void assignDefID(llvm::Value *Def);

public:
  LLVMNodeIterator def_begin() { return LLVMNodeIterator::begin(*this, &UseDefBuilder::isDef, getConstructedFunctions()); }
  LLVMNodeIterator def_end()   { return LLVMNodeIterator::end(*this, &UseDefBuilder::isDef, getConstructedFunctions()); }
  LLVMNodeIterator use_begin() { return LLVMNodeIterator::begin(*this, &UseDefBuilder::isUse, getConstructedFunctions()); }
  LLVMNodeIterator use_end()   { return LLVMNodeIterator::end(*this, &UseDefBuilder::isUse, getConstructedFunctions()); }
  GlobalInitIterator glob_begin() { return GlobalInitIterator::begin(getGlobals()); }
  GlobalInitIterator glob_end()   { return GlobalInitIterator::end(getGlobals()); }
};

} // namespace dg

#endif