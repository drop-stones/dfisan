#ifndef LLVM_ANALYSIS_SVF_USEDEFANALYSIS_USEDEFSVFGBUILDER_H
#define LLVM_ANALYSIS_SVF_USEDEFANALYSIS_USEDEFSVFGBUILDER_H

#include "MSSA/SVFGBuilder.h"

namespace SVF {

class UseDefSVFGBuilder : public SVFGBuilder {
public:
  using SVFGNodeSet = Set<const SVFGNode *>;
  using SVFGEdgeSet = Set<SVFGEdge *>;

  /// Constructor
  UseDefSVFGBuilder() : SVFGBuilder(true) {}

  /// Destructor
  virtual ~UseDefSVFGBuilder() {}

protected:
  /// Build SVFG for Use-Def analysis.
  virtual void buildSVFG();

  /// Return true if this is a strong update STORE statement.
  bool isStrongUpdate(const SVFGNode *node, NodeID &singleton, BVDataPTAImpl *pta);

private:
  /// Remove direct value-flow edge to a dereference point for Use-Def calculation.
  /// For example, given two statements: p = alloc; q = *p, the direct SVFG edge between them is deleted
  /// because those edges only stand for values used at the dereference points but they can not pass the value to other definitions.
  void rmDerefDirSVFGEdges(BVDataPTAImpl *pta);

  /// Remove Incoming Edge for strong-update (SU) store statement
  /// because the SU node does not receive indirect value.
  void rmIncomingEdgeForSUStore(BVDataPTAImpl *pta);

  /// Remove direct Outgoing Edge for load statement
  /// because we don't care about Load -> Store dependencies.
  void rmDirOutgoingEdgeForLoad(BVDataPTAImpl *pta);
};

} // namespace SVF

#endif