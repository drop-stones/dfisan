//===-- UseDefChain.h - Use-Def Chain definition ----------------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//
///
/// This file contains the declaration of the UseDefSVFGBuilder class,
/// which creates an SVFG and removes some unrelated edges.
///
//===----------------------------------------------------------------------===//

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

  /// Print all SVFG nodes of a specified type.
  void printSVFGNodes(SVFGNode::VFGNodeK Type) const;

protected:
  /// Build SVFG for Use-Def analysis.
  virtual void buildSVFG() override;

  /// Return true if this is a strong update STORE statement.
  bool isStrongUpdate(const SVFGNode *Node, NodeID &Singleton, BVDataPTAImpl *Pta);

private:
  StringRef ModuleName;

  /// Merge zeroinitializer nodes of global array
  /// because array initialization has one unique DefID.
  void mergeGlobalArrayInitializer(SVFIR *Pag);

  /// Remove direct value-flow edge to a dereference point for Use-Def calculation.
  /// For example, given two statements: p = alloc; q = *p, the direct SVFG edge between them is deleted
  /// because those edges only stand for values used at the dereference points but they can not pass the value to other definitions.
  void rmDerefDirSVFGEdges(BVDataPTAImpl *Pta);

  /// Remove Incoming Edge for strong-update (SU) store statement
  /// because the SU node does not receive indirect value.
  void rmIncomingEdgeForSUStore(BVDataPTAImpl *Pta);

  /// Remove direct Outgoing Edge for load statement
  /// because we don't care about Load -> Store dependencies.
  void rmDirOutgoingEdgeForLoad(BVDataPTAImpl *Pta);

  /// Remove direct Edges from Load-Memcpy to Store-Memcpy
  /// because Load-Memcpy cannot be attacked
  /// and Store-Memcpy must be strong updates.
  void rmDirEdgeFromMemcpyToMemcpy(BVDataPTAImpl *Pta);
};

} // namespace SVF

#endif