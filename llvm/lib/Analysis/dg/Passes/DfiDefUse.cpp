#include "DfiDefUse.h"

namespace dg {

bool DfiDefUseAnalysis::runOnNode(LLVMNode *Node, LLVMNode *Prev) {
  Value *Val = Node->getKey();
  if (!ProtectInfo.isSelectiveDfi()) // Entire analysis
    return LLVMDefUseAnalysis::runOnNode(Node, Prev);
  if (!RD->isUse(Val))
    return LLVMDefUseAnalysis::runOnNode(Node, Prev);

  // Selective DataFlow analysis
  auto *RWNode = RD->getNode(Val);
  for (auto &Use : RWNode->getUses()) {
    auto *UseVal = RD->getValue(Use.target);
    if (ProtectInfo.hasValue((Value *)UseVal)) {
      llvm::errs() << "runOnNode: " << *Val << "\n";
      ProtectInfo.insertUse(Val);
      auto Ret = LLVMDefUseAnalysis::runOnNode(Node, Prev);
      for (auto *Def : RD->getLLVMDefinitions(Val)) {
        ProtectInfo.insertDef(Def);
      }
      return Ret;
    }
  }
  return false;
}

void DfiDefUseAnalysis::addDataDependencies(LLVMNode *Node) {
  LLVMDefUseAnalysis::addDataDependencies(Node);
}

} // namespace dg