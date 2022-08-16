#include "dg/Passes/DfiMemorySSA.h"
#include "dg/MemorySSA/MemorySSA.h"
#include "dg/Passes/DfiUtils.h"

#include <llvm/Support/Debug.h>
#define DEBUG_TYPE "dfi-memoryssa"

namespace dg {
namespace dda {

std::vector<RWNode *>
DfiMemorySSATransformation::findDefinitions(RWBBlock *block, const DefSite &ds) {
  LLVM_DEBUG(llvm::errs() << "DfiMemorySSATransformation::" << __func__ << " with RWBBlock(" << block->getID() << ")\n");
  return MemorySSATransformation::findDefinitions(block, ds);
}

std::vector<RWNode *>
DfiMemorySSATransformation::findDefinitions(RWNode *node, const DefSite &ds) {
  LLVM_DEBUG(llvm::errs() << "DfiMemorySSATransformation::" << __func__ << " with RWNode(" << node->getID() << ") + DefSite(" << ds.target << ")\n");
  return MemorySSATransformation::findDefinitions(node, ds);
}

std::vector<RWNode *>
DfiMemorySSATransformation::findDefinitions(RWNode *node) {
  LLVM_DEBUG(llvm::errs() << "DfiMemorySSATransformation::" << __func__ << " with RWNode(" << node->getID() << ")\n");
  std::vector<RWNode *> ret;
  if (node->getID() == 1051 || node->getID() == 1053 || node->getID() == 1055) {
    addCurrentDebugType(DEBUG_TYPE);
    node->getBBlock()->dump();
    LLVM_DEBUG(llvm::errs() << "Block(" << node->getBBlock()->getID() << ")\n");
    LLVM_DEBUG(
      for (auto *Node : node->getBBlock()->getNodes()) {
          llvm::errs() << " - Node(" << Node->getID() << "): ";
          if (RWBuilder->getValue(Node) != nullptr)
            llvm::errs() << *RWBuilder->getValue(Node) << "\n";
          else
            llvm::errs() << "no value\n";
      }
    );
    ret = MemorySSATransformation::findDefinitions(node);
    LLVM_DEBUG(
      llvm::errs() << node->getID() << ": ";
      printRWNodeVector(ret, RWBuilder, getGraph());
    );
    popCurrentDebugType(DEBUG_TYPE);
  } else {
    ret = MemorySSATransformation::findDefinitions(node);
  }

  return ret;
}

void DfiMemorySSATransformation::addUncoveredFromPredecessors(
  RWBBlock *block, Definitions &D, const DefSite &ds, std::vector<RWNode *> &defs) {
  LLVM_DEBUG(llvm::errs() << "DfiMemorySSATransformation::" << __func__ << "\n");
  LLVM_DEBUG(
    for (auto &interval : D.uncovered(ds))
      llvm::errs() << " - start: " << interval.start.offset << ", length: " << interval.length().offset << "\n";
  );
  LLVM_DEBUG(D.dump(); std::flush(std::cout));
  LLVM_DEBUG(
    for (auto &definition : D.definitions) {
      llvm::errs() << "definition: ID(" << definition.first->getID() << "), address(" << definition.first << "), " << *RWBuilder->getValue(definition.first) << "\n";
    }
  );
  LLVM_DEBUG(llvm::errs() << "defs: "; printRWNodeVector(defs));

  MemorySSATransformation::addUncoveredFromPredecessors(block, D, ds, defs);
}

RWNode *
DfiMemorySSATransformation::createPhi(Definitions &D, const DefSite &ds, RWNodeType type) {
  LLVM_DEBUG(llvm::errs() << "DfiMemorySSATransformation::" << __func__ << "\n");
  LLVM_DEBUG(llvm::errs() << " - DefSite: target(" << *RWBuilder->getValue(ds.target) << ")\n");
  return MemorySSATransformation::createPhi(D, ds, type);
}

Definitions &DfiMemorySSATransformation::getBBlockDefinitions(RWBBlock *b, const DefSite *ds) {
  LLVM_DEBUG(llvm::errs() << "DfiMemorySSATransformation::" << __func__ << "\n");
  if (getBBlockInfo(b).isCallBlock()) {
    LLVM_DEBUG(llvm::errs() << " - isCallBlock\n");
  } else {
    LLVM_DEBUG(llvm::errs() << " - is not CallBlock\n");
  }
  return MemorySSATransformation::getBBlockDefinitions(b, ds);
}

std::vector<RWNode *>
DfiMemorySSATransformation::getDefinitions(RWNode *where, RWNode *mem,
                                           const Offset &off, const Offset &len) {
  LLVM_DEBUG(llvm::errs() << "DfiMemorySSATransformation::" << __func__ << "\n");
  return MemorySSATransformation::getDefinitions(where, mem, off, len);
}

void DfiMemorySSATransformation::run() {
  LLVM_DEBUG(llvm::errs() << "DfiMemorySSATransformation::" << __func__ << "\n");
  MemorySSATransformation::run();
}

} // namespace dda
} // namespace dg
