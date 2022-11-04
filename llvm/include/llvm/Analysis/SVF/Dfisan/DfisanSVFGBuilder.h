#ifndef DFISAN_SVFGBUILDER_H_
#define DFISAN_SVFGBUILDER_H_

#include "MTA/FSMPTA.h"

namespace SVF {

class DfisanSVFGBuilder : public MTASVFGBuilder {
public:
  /// Constructor
  DfisanSVFGBuilder(MHP *m, LockAnalysis *la) : MTASVFGBuilder(m, la) {}

  /// Destructor
  virtual ~DfisanSVFGBuilder() {}

  SVFG *buildFullSVFG(BVDataPTAImpl *pta);

  /// Build Memory SSA for dfisan
  virtual MemSSA *buildMSSA(BVDataPTAImpl *pta, bool ptrOnlyMSSA) override;

protected:
  /// Re-write create SVFG method
  virtual void buildSVFG() override;

private:
  SVFG *build(BVDataPTAImpl *pta, VFG::VFGK kind);

  /// Return true if this is a strong update STORE statement.
  bool isStrongUpdate(const SVFGNode *Node, NodeID &Singleton, BVDataPTAImpl *Pta);

  /// Remove incoming edge for strong-update store instruction.
  /// because the SU node does not receive any indirect values.
  void rmIncomingEdgeForSUStore(BVDataPTAImpl *Pta);

  /// Remove store/load edge for byval pointer
  void rmByvalEdge(BVDataPTAImpl *Pta);

  /// Remove all direct edges
  /// because direct edge means def/use of top-level variables,
  /// but we need def/use of address-taken variables (indirect edges).
  void rmAllDirSVFGEdge();
};

} // namespace SVF

#endif