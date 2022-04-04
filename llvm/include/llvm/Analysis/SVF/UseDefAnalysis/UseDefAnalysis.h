#ifndef LLVM_ANALYSIS_SVF_USEDEFANALYSIS_USEDEFANALYSIS_H
#define LLVM_ANALYSIS_SVF_USEDEFANALYSIS_USEDEFANALYSIS_H

#include "UseDefAnalysis/UseDefSVFGBuilder.h"

namespace SVF {

class UseDefAnalysis {
protected:
  UseDefSVFGBuilder SvfgBuilder;
  SVFG *Svfg;
  //UseDefChain UseDef;

public:
  /// Constructor
  UseDefAnalysis() : Svfg(nullptr) {}

  /// Destructor
  ~UseDefAnalysis() {
    if (Svfg != nullptr)
      delete Svfg;
    Svfg = nullptr;
  }

  /// Start Use-Def Analysis
  void analyze(SVFModule *M);

  /// Initialize Analysis
  void initialize(SVFModule *M);

  /// Get SVFG
  inline SVFG *getSVFG() const {
    return Svfg;
  }

  /// Get Use-Def
  // inline UseDefChain &getUseDef();
};

} // namespace SVF

#endif