#include "Graphs/SVFG.h"
#include "Graphs/SVFGStat.h"
#include "Dfisan/DfisanSVFG.h"

#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "dfisan-svfg"

using namespace SVF;
using namespace SVFUtil;

void DfisanSVFG::buildSVFG() {
  LLVM_DEBUG(llvm::dbgs() << "DfisanSVFG::" << __func__ << "\n");

  stat->startClk();

  stat->ATVFNodeStart();
  addSVFGNodesForAddrTakenVars();
  stat->ATVFNodeEnd();

  stat->indVFEdgeStart();
  connectIndirectSVFGEdges();
  stat->indVFEdgeEnd();
}
