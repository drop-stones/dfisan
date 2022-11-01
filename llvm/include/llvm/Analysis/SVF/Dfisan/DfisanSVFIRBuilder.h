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

  /// Get the base value of (i8 *src and i8* dst) for external argument (e.g., memcpy(i8* dst, i8* src, int size))
  virtual const Value *getBaseValueForExtArg(const Value *V) override;

private:
  NodeID getZeroValNode();
  void addComplexConsForDfisanExt(NodeID Dst, NodeID Src);

  /// Impl of global init handling
  void InitialGlobalForDfisan(const GlobalVariable *gvar, Constant *C, u32_t offset, Constant *Base = nullptr);
};

} // namespace SVF

#endif
