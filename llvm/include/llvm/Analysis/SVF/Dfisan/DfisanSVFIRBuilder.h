#ifndef DFISAN_SVFIRBUILDER_H_
#define DFISAN_SVFIRBUILDER_H_

#include "SVF-FE/SVFIRBuilder.h"

namespace SVF {

class DfisanSVFIRBuilder : public SVFIRBuilder {
public:
  /// Constructor
  DfisanSVFIRBuilder() : SVFIRBuilder() {}

  /// Destructor
  virtual ~DfisanSVFIRBuilder() {}

protected:
  /// Handle global init
  void InitialGlobal(const GlobalVariable *gvar, Constant *C, u32_t offset) override;

  /// Handle external call
  virtual void handleExtCall(CallSite cs, const SVFFunction *F) override;

private:
  NodeID getZeroValNode();
  void addComplexConsForDfisanExt(NodeID Dst, NodeID Src);
};

} // namespace SVF

#endif
