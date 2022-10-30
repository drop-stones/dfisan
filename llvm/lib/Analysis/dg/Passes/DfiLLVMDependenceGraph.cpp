#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>

#include "dg/Passes/DfiLLVMDependenceGraph.h"
#include "DfiDefUse.h"

namespace dg {

void DfiLLVMDependenceGraph::addDefUseEdges(bool preserveDbg) {
  DfiDefUseAnalysis DUA(this, DDA, PTA, ProtectInfo);
  DUA.run();

  if (preserveDbg) {
    using namespace llvm;

    for (const auto &it : getConstructedFunctions()) {
      LLVMDependenceGraph *dg = it.second;
      for (auto &I : instructions(cast<Function>(it.first))) {
        Value *val = nullptr;
        if (auto *DI = dyn_cast<DbgDeclareInst>(&I))
          val = DI->getAddress();
        else if (auto *DI = dyn_cast<DbgValueInst>(&I))
          val = DI->getValue();
#if LLVM_VERSION_MAJOR > 5
        else if (auto *DI = dyn_cast<DbgAddrIntrinsic>(&I))
          val = DI->getAddress();
#endif

        if (val) {
          auto *nd = dg->getNode(&I);
          auto *ndop = dg->getNode(val);
          assert(nd && "Do not have a node for a dbg intrinsic");
          assert(ndop && "Do not have a node for an operand of a dbg "
                          "intrinsic");
          // add a use edge such that we preserve
          // the debugging intrinsic when we preserve
          // the value it is talking about
          nd->addUseDependence(ndop);
        }
      }
    }
  }
}

} // namespace dg
