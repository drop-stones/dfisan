#include "dg/Passes/UseDefBuilder.h"
#include "dg/Passes/DfiUtils.h"
#include "dg/llvm/LLVMDependenceGraph.h"

using namespace dg;

bool
UseDefBuilder::isDef(llvm::Value *Val) {
  return getDDA()->isDef(Val);
}
bool
UseDefBuilder::isUse(llvm::Value *Val) {
  return getDDA()->isUse(Val);
}

void
UseDefBuilder::dump(llvm::raw_ostream &OS) {
  OS << __func__ << "\n";
  auto *DDA = getDDA();
  for (auto GI = glob_begin(); GI != glob_end(); GI++) {
    const llvm::Value *GlobalInit = DDA->getValue(*GI);
    OS << "GlobalInit: " << *GlobalInit << "\n";
  }
  for (auto DI = def_begin(); DI != def_end(); DI++) {
    LLVMNode *DefNode = *DI;
    llvm::Value *Def = DefNode->getValue();
    dda::RWNode *WriteNode = DDA->getNode(Def);
    OS << "Def: " << *Def << "\n";
    OS << " - Define:\n";
    for (auto &Define : WriteNode->getDefines()) {
      OS << "\t- [off: " << Define.offset.offset << ", len: " << Define.len.offset << "]: " << *DDA->getValue(Define.target) << "\n";
    }
    OS << " - Overwrite:\n";
    for (auto &Overwrite : WriteNode->getOverwrites()) {
      OS << "\t- [off: " << Overwrite.offset.offset << ", len: " << Overwrite.len.offset << "]: " << *DDA->getValue(Overwrite.target) << "\n";
    }
    OS << " - Use:\n";
    for (auto &Use : WriteNode->getUses()) {
      OS << "\t- [off: " << Use.offset.offset << ", len: " << Use.len.offset << "]: " << *DDA->getValue(Use.target) << "\n";
    }
  }
  for (auto UI = use_begin(); UI != use_end(); UI++) {
    LLVMNode *UseNode = *UI;
    llvm::Value *Use = UseNode->getValue();
    dda::RWNode *ReadNode = DDA->getNode(Use);
    OS << "Use: " << *Use << "\n";
    OS << " - Define:\n";
    for (auto &Define : ReadNode->getDefines()) {
      OS << "\t- [off: " << Define.offset.offset << ", len: " << Define.len.offset << "]: " << *DDA->getValue(Define.target) << "\n";
    }
    OS << " - Overwrite:\n";
    for (auto &Overwrite : ReadNode->getOverwrites()) {
      OS << "\t- [off: " << Overwrite.offset.offset << ", len: " << Overwrite.len.offset << "]: " << *DDA->getValue(Overwrite.target) << "\n";
    }
    OS << " - Use:\n";
    for (auto &Use : ReadNode->getUses()) {
      OS << "\t- [off: " << Use.offset.offset << ", len: " << Use.len.offset << "]: " << *DDA->getValue(Use.target) << "\n";
    }
  }
}
