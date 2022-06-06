#ifndef LLVM_ANALYSIS_SVF_USEDEFANALYSIS_USEDEFSVFIRBUILDER_H
#define LLVM_ANALYSIS_SVF_USEDEFANALYSIS_USEDEFSVFIRBUILDER_H

#include "SVF-FE/SVFIRBuilder.h"
#include "llvm/ADT/SmallVector.h"

namespace SVF {

class UseDefSVFIRBuilder : public SVFIRBuilder {
public:
  /// Constructor
  UseDefSVFIRBuilder() : SVFIRBuilder() {}

  /// Destructor
  virtual ~UseDefSVFIRBuilder() {}

protected:
  /// Initialze global variables
  void InitialGlobal(const GlobalVariable *Gvar, Constant *Const, u32_t Offset) override;
};

} // namespace SVF

#endif