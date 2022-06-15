//===-- UseDefAnalysis.h - Use-Def Analysis definition ----------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//
///
/// This file contains the declaration of the UseDefAnalysis class,
/// which create a SVFG, analyze it, and save the results to UseDefChain class.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_SVF_USEDEFANALYSIS_USEDEFANALYSIS_H
#define LLVM_ANALYSIS_SVF_USEDEFANALYSIS_USEDEFANALYSIS_H

#include "UseDefAnalysis/UseDefSVFGBuilder.h"
#include "UseDefAnalysis/UseDefChain.h"
#include "UseDefAnalysis/UseDefSVFIRBuilder.h"

namespace SVF {

class UseDefAnalysis {
protected:
  UseDefSVFIRBuilder IRBuilder;
  UseDefSVFGBuilder SvfgBuilder;
  SVFIR *Pag;
  SVFG *Svfg;
  UseDefChain *UseDef;

public:
  /// Constructor
  UseDefAnalysis() : Pag(nullptr), Svfg(nullptr), UseDef(nullptr) {}

  /// Destructor
  ~UseDefAnalysis() {
    if (Svfg != nullptr)
      delete Svfg;
    if (UseDef != nullptr)
      delete UseDef;

    Svfg   = nullptr;
    UseDef = nullptr;
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
  inline UseDefChain *getUseDef() const {
    return UseDef;
  }
};

} // namespace SVF

#endif