#include "Dfisan/DfisanMemSSA.h"
#include "Dfisan/DfisanExtAPI.h"
#include "MemoryModel/PointerAnalysisImpl.h"
#include "SVF-FE/LLVMUtil.h"

using namespace SVF;
using namespace SVFUtil;
using namespace LLVMUtil;

/*!
 * Create mu/chi according to memory regions
 * collect used mrs in usedRegs and construction map from region to BB for prune SSA phi insertion
 */
void DfisanMemSSA::createMUCHI(const SVFFunction &fun) {

  SVFIR* pag = pta->getPAG();

  // 1. create mu/chi
  //	insert a set of mus for memory regions at each load
  //    + load external fun
  //  inset a set of chis for memory regions at each store
  //    + store external fun

  // 2. find global names (region name before renaming) of each memory region,
  // collect used mrs in usedRegs, and collect its def basic block in reg2BBMap
  // in the form of mu(r) and r = chi (r)
  // a) mu(r):
  // 		if(r \not\in varKills) global = global \cup r
  // b) r = chi(r):
  // 		if(r \not\in varKills) global = global \cup r
  //		varKills = varKills \cup r
  //		block(r) = block(r) \cup bb_{chi}

  /// get all reachable basic blocks from function entry
  /// ignore dead basic blocks
  BBList reachableBBs;
  getFunReachableBBs(fun.getLLVMFun(),getDT(fun),reachableBBs);

  for (BBList::const_iterator iter = reachableBBs.begin(), eiter = reachableBBs.end();
        iter != eiter; ++iter)
  {
    const BasicBlock* bb = *iter;
    varKills.clear();
    for (BasicBlock::const_iterator it = bb->begin(), eit = bb->end();
            it != eit; ++it)
    {
      const Instruction* inst = &*it;
      if(mrGen->hasSVFStmtList(inst))
      {
        SVFStmtList& pagEdgeList = mrGen->getPAGEdgesFromInst(inst);
        for (SVFStmtList::const_iterator bit = pagEdgeList.begin(),
              ebit = pagEdgeList.end(); bit != ebit; ++bit)
        {
          const PAGEdge* inst = *bit;
          if (const LoadStmt* load = SVFUtil::dyn_cast<LoadStmt>(inst)) {
            AddLoadMU(bb, load, mrGen->getLoadMRSet(load));
          } else if (const StoreStmt* store = SVFUtil::dyn_cast<StoreStmt>(inst)) {
            AddStoreCHI(bb, store, mrGen->getStoreMRSet(store));
          }
        }
      }
      if (isNonInstricCallSite(inst))
      {
        // Skip mu/chi insertions if the callee is DfisanExtAPI.
        const SVFFunction *Callee = getCallee(inst);
        if (Callee != nullptr && DfisanExtAPI::getDfisanExtAPI()->isExtFun(Callee))
          continue;

        const CallICFGNode* cs = pag->getICFG()->getCallICFGNode(inst);
        if(mrGen->hasRefMRSet(cs))
          AddCallSiteMU(cs,mrGen->getCallSiteRefMRSet(cs));

        if(mrGen->hasModMRSet(cs))
          AddCallSiteCHI(cs,mrGen->getCallSiteModMRSet(cs));
      }
    }
  }

  // create entry chi for this function including all memory regions
  // initialize them with version 0 and 1 r_1 = chi (r_0)
  for (MRSet::iterator iter = usedRegs.begin(), eiter = usedRegs.end();
        iter != eiter; ++iter)
  {
    const MemRegion* mr = *iter;
    // initialize mem region version and stack for renaming phase
    mr2CounterMap[mr] = 0;
    mr2VerStackMap[mr].clear();
    ENTRYCHI* chi = new ENTRYCHI(&fun, mr);
    chi->setOpVer(newSSAName(mr,chi));
    chi->setResVer(newSSAName(mr,chi));
    funToEntryChiSetMap[&fun].insert(chi);

    /// if the function does not have a reachable return instruction from function entry
    /// then we won't create return mu for it
    if(functionDoesNotRet(fun.getLLVMFun()) == false)
    {
      RETMU* mu = new RETMU(&fun, mr);
      funToReturnMuSetMap[&fun].insert(mu);
    }

  }
}
