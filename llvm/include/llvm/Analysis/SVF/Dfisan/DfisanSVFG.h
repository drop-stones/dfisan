#ifndef DFISAN_SVFG_H_
#define DFISAN_SVFG_H_

#include "Graphs/SVFG.h"

namespace SVF {

class DfisanSVFG : public SVFG {
  friend class DfisanSVFGBuilder;

protected:
  /// Constructor
  DfisanSVFG(MemSSA *mssa, VFGK k) : SVFG(mssa, k) {}

  /// Start building SVFG
  virtual void buildSVFG() override;

public:
  /// Destructor
  virtual ~DfisanSVFG() { SVFG::destroy(); }

protected:
  /// Create SVFG nodes for protection targets
  void addSVFGNodesForProtectionTargets();
  /// Connect direct SVFG edges between two SVFG nodes of protection targets
  void connectIndirectSVFGEdgesForProtectionTargets();
};

} // namespace SVF

#endif
