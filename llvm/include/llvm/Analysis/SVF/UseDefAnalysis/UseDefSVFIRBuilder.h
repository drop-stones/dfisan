#ifndef LLVM_ANALYSIS_SVF_USEDEFANALYSIS_USEDEFSVFIRBUILDER_H
#define LLVM_ANALYSIS_SVF_USEDEFANALYSIS_USEDEFSVFIRBUILDER_H

#include "SVF-FE/SVFIRBuilder.h"

namespace SVF {

class UseDefSVFIRBuilder : public SVFIRBuilder {
public:
  /// Constructor
  UseDefSVFIRBuilder() : SVFIRBuilder() {}

  /// Destructor
  virtual ~UseDefSVFIRBuilder() {}

  /// Add global struct zeroinitializer.
  /// Called only be addGlobalAggregateTypeInitialization().
  void addGlobalStructZeroInitializer(SVFIR *Pag, const llvm::GlobalVariable *GlobalVar);

  /// Add PAG Node and StoreStmt to PAG for global aggregate type initialization.
  void addGlobalAggregateTypeInitializationNodes();
};

} // namespace SVF

#endif