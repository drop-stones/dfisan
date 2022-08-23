#include "dg/Passes/DfiUtils.h"
#include "llvm/ReadWriteGraph/LLVMReadWriteGraphBuilder.h"

#include <llvm/IR/Instruction.h>
#include <llvm/IR/DebugLoc.h>
#include <llvm/IR/DebugInfoMetadata.h>

using namespace llvm;

namespace dg {

static constexpr unsigned MemoryAllocFuncCount = 3;
static constexpr char MemoryAllocFunc[MemoryAllocFuncCount][10] = {"malloc", "calloc", "realloc"};

bool isMemoryAllocCall(Value *V) {
  if (const auto *Call = dyn_cast<llvm::CallInst>(V)) {
    const Function *Func = Call->getCalledFunction();
    for (unsigned Idx = 0; Idx < MemoryAllocFuncCount; Idx++) {
      if (Func->getName() == MemoryAllocFunc[Idx])
        return true;
    }
  }
  return false;
}

void printUseDefFromDebugLoc(dda::LLVMDataDependenceAnalysis *DDA, Value *Val, unsigned int Col, unsigned int Line) {
  if (auto *Inst = dyn_cast<Instruction>(Val)) {
    auto Loc = Inst->getDebugLoc();
    if (Loc) {
      if (Loc->getColumn() == Col && Loc->getLine() == Line) {
        llvm::errs() << "Found ID(" << DDA->getNode(Val)->getID() << ") " << *Inst << "\n";
        for (auto *Def : DDA->getLLVMDefinitions(Val)) {
          llvm::errs() << " - " << *Def << "\n";
        }
      }
    }
  }
}

void printDefinitionsFromDebugLoc(dda::RWNode *Use, std::vector<dda::RWNode *> &Defs, unsigned int Col, unsigned int Line) {
  auto *Val = Use->getUserData<Value>();
  if (Val == nullptr)
    return;
  if (auto *Inst = dyn_cast<Instruction>(Val)) {
    auto Loc = Inst->getDebugLoc();
    if (Loc) {
      if (Loc->getColumn() == Col && Loc->getLine() == Line) {
        llvm::errs() << "Found " << *Inst << "\n";
        for (auto *Def : Defs) {
          auto *DefVal = Use->getUserData<Value>();
          if (DefVal != nullptr)
            llvm::errs() << " - " << *DefVal << "\n";
        }
      }
    }
  }
}

void printRWNode(dda::LLVMDataDependenceAnalysis *DDA, dda::RWNode *Node) {
  if (Node == nullptr)
    return;
  auto *Val = DDA->getValue(Node);
  llvm::errs() << __func__ << ": Node(" << Node->getID() << ") " << *Val << "\n";
}

// copy from MemroySSA.cpp
static void recGatherNonPhisDefs(dda::RWNode *phi,
                                 dg::ADT::SparseBitvectorHashImpl &phis,
                                 dg::ADT::SparseBitvectorHashImpl &ret,
                                 bool intraproc = false) {
    assert(phi->isPhi());
    // set returns the previous value, so if its 'true',
    // we already had the phi
    if (phis.set(phi->getID()))
        return; // we already visited this phi

    for (auto *n : phi->defuse) {
        if (!n->isPhi()) {
            ret.set(n->getID());
        } else {
            if (intraproc && n->isInOut()) {
                ret.set(n->getID());
            } else {
                recGatherNonPhisDefs(n, phis, ret, intraproc);
            }
        }
    }
}

void printRWNodeVector(std::vector<dda::RWNode *> &Nodes, const dda::LLVMReadWriteGraphBuilder *RWBuilder, dda::ReadWriteGraph *Graph) {
  if (Nodes.empty())
    return;
  llvm::errs() << "{ ";
  for (unsigned Idx = 0; Idx < Nodes.size() - 1; Idx++) {
    llvm::errs() << Nodes[Idx]->getID() << ", ";
  }
  llvm::errs() << Nodes.back()->getID() << " }\n";

  if (RWBuilder != nullptr) {
    for (auto *Node : Nodes) {
      auto *Val = RWBuilder->getValue(Node);
      if (Val != nullptr) {
        llvm::errs() << " - ID(" << Node->getID() << "): " << *Val << "\n";
      } else if (Node->isPhi()) {
        llvm::errs() << " - ID(" << Node->getID() << "): Phi\n";
        dg::ADT::SparseBitvectorHashImpl ret;
        dg::ADT::SparseBitvectorHashImpl phis;
        recGatherNonPhisDefs(Node, phis, ret, false);
        for (auto i : ret) {
          llvm::errs() << "   - ID(" << i << "): " << *RWBuilder->getValue(Graph->getNode(i)) << "\n";
        }
      } else {
        llvm::errs() << " - ID(" << Node->getID() << "): no value\n";
      }
    }
  }
}

void printRWBBlock(dda::RWBBlock *Block, const dda::LLVMReadWriteGraphBuilder *RWBuilder) {
  llvm::errs() << "Block(" << Block->getID() << ")\n";
  for (auto *Node : Block->getNodes()) {
      llvm::errs() << " - Node(" << Node->getID() << "): ";
      if (RWBuilder->getValue(Node) != nullptr)
        llvm::errs() << *RWBuilder->getValue(Node) << "\n";
      else
        llvm::errs() << "no value\n";
  }
}

} // namespace dg