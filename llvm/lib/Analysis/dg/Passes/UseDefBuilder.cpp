#include "dg/Passes/UseDefBuilder.h"
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

// TODO: Support heap object
void
UseDefBuilder::findDfiProtectTargets() {
  using namespace llvm;
  // Annotated global variables
  if (GlobalVariable *GlobAnnotation = M->getGlobalVariable("llvm.global.annotations")) {
    for (Value *Op : GlobAnnotation->operands()) {  // Metadata are stored in an array of struct of metadata
      if (ConstantArray *CA = dyn_cast<ConstantArray>(Op)) {
        for (Value *CAOp : CA->operands()) {
          if (ConstantStruct *CS = dyn_cast<ConstantStruct>(CAOp)) {
            if (CS->getNumOperands() >= 2) {
              Value *AnnotatedVal = CS->getOperand(0)->getOperand(0);
              if (GlobalVariable *GAnn = dyn_cast<GlobalVariable>(CS->getOperand(1)->getOperand(0))) {
                if (ConstantDataArray *Arr = dyn_cast<ConstantDataArray>(GAnn->getOperand(0))) {
                  StringRef AnnStr = Arr->getAsString();
                  if (AnnStr.startswith(DfiProtectAnn)) {
                    llvm::errs() << "Annotation: " << AnnStr << ", GlobalVal: " << *AnnotatedVal << "\n";
                    ProtectInfo.insertGlobal(AnnotatedVal);
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  for (Function &Func : M->getFunctionList()) {
    for (Instruction &Inst : instructions(&Func)) {
      if (IntrinsicInst *Intrinsic = dyn_cast<IntrinsicInst>(&Inst)) {
        Function *Callee = Intrinsic->getCalledFunction();
        if (Callee->getName() == "llvm.var.annotation") {
          Value *AnnotatedVal = Intrinsic->getArgOperand(0)->stripPointerCasts();
          if (GlobalVariable *Ann = dyn_cast<GlobalVariable>(Intrinsic->getArgOperand(1)->stripPointerCasts())) {
            if (ConstantDataArray *Arr = dyn_cast<ConstantDataArray>(Ann->getOperand(0))) {
              StringRef AnnStr = Arr->getAsString();
              llvm::errs() << "Annotation: " << AnnStr << ", annotatedVal: " << *AnnotatedVal << "\n";
              ProtectInfo.insertLocal(AnnotatedVal);
            }
          }
        }
      }
    }
  }
}

void
UseDefBuilder::assignDefIDs() {
  // Assign DefID to global initializations.
  for (auto GI = glob_begin(), GE = glob_end(); GI != GE; GI++) {
    auto *GlobInit = (llvm::Value *)getDDA()->getValue(*GI);
    assert(isDef(GlobInit));
    assignDefID(GlobInit);
  }

  // Assign DefID to store instructions.
  for (auto DI = def_begin(), DE = def_end(); DI != DE; DI++) {
    auto *Def = (*DI)->getValue();
    assert(isDef(Def));
    assignDefID(Def);
  }
}

void
UseDefBuilder::assignDefID(llvm::Value *Def) {
  static DefID CurrID = 1;
  if (DefToInfo.count(Def) == 0) {
    DefToInfo.emplace(Def, CurrID);

    if (CurrID == USHRT_MAX) {
      llvm::errs() << "DefID overflow!!\n";
      CurrID = 1;
    } else {
      CurrID++;
    }
  }
}

bool
UseDefBuilder::hasDefID(llvm::Value *Key) {
  return DefToInfo.count(Key) != 0;
}

DefID
UseDefBuilder::getDefID(llvm::Value *Key) {
  assert(hasDefID(Key));
  return DefToInfo[Key].ID;
}

void
UseDefBuilder::printUseDef(llvm::raw_ostream &OS) {
  OS << __func__ << "\n";
  for (auto UI = use_begin(), UE = use_end(); UI != UE; UI++) {
    auto *Use = (*UI)->getValue();
    OS << "Use: " << *Use << "\n";
    for (auto *Def : getDDA()->getLLVMDefinitions(Use)) {
      OS << " - DefID[" << getDefID(Def) << "]: " << *Def << "\n";
    }
  }
}

void
UseDefBuilder::printDefInfoMap(llvm::raw_ostream &OS) {
  OS << __func__ << "\n";
  for (const auto &Iter : DefToInfo) {
    auto *Val = Iter.first;
    auto &DefInfo = Iter.second;

    OS << "DefID[" << DefInfo.ID << "]: " << *Val << "\n";
  }
}

void
UseDefBuilder::printProtectInfo(llvm::raw_ostream &OS) {
  ProtectInfo.dump(OS);
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
