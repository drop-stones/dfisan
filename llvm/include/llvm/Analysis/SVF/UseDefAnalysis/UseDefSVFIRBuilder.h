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

  /// Add global struct zeroinitializer.
  /// Called only be addGlobalAggregateTypeInitializationNodes().
  void addGlobalStructZeroInitializer(const llvm::GlobalVariable *BaseVal, llvm::StructType *StructTy, const llvm::Constant *StructInit, unsigned &AccumulateOffset);

  /// Add global struct member initializations.
  /// Called only be addGlobalAggregateTypeInitializationNodes().
  void addGlobalStructMemberInitializer(const llvm::GlobalVariable *BaseVal, llvm::StructType *StructTy, const llvm::ConstantStruct *StructConst, unsigned &AccumulateOffset, llvm::SmallVector<unsigned, 8> &OffsetVec);

  /// Add PAG Node and StoreStmt to PAG for global aggregate type initialization.
  void addGlobalAggregateTypeInitializationNodes();

protected:
  /// Initialze global variables
  void InitialGlobal(const GlobalVariable *Gvar, Constant *Const, u32_t Offset) override;
};

} // namespace SVF

#endif