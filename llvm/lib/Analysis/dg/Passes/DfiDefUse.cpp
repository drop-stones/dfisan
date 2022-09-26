#include "DfiDefUse.h"

namespace dg {

bool DfiDefUseAnalysis::runOnNode(LLVMNode *Node, LLVMNode *Prev) {
  Value *Val = Node->getKey();

  if (!RD->isUse(Val))
    return LLVMDefUseAnalysis::runOnNode(Node, Prev);
  assert(isa<Instruction>(Val) && "runOnNode: not instruction use");
  Instruction *Inst = dyn_cast<Instruction>(Val);

  // Selective DataFlow analysis
  auto *RWNode = RD->getNode(Val);
  for (auto &Use : RWNode->getUses()) {
    auto *UseVal = RD->getValue(Use.target);
    if (ProtectInfo->hasTarget((llvm::Value *)UseVal)) {
      // llvm::errs() << "runOnNode: " << *Val << "\n";
      // llvm::errs() << " - Use: " << *UseVal << "\n";
      ProtectInfo->insertUse(Inst);
      auto Ret = LLVMDefUseAnalysis::runOnNode(Node, Prev);
      for (auto *Def : RD->getLLVMDefinitions(Val)) {
        ProtectInfo->insertDef((Value *)Def);
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