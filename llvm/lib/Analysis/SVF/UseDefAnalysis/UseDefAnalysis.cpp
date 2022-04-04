#include "UseDefAnalysis/UseDefAnalysis.h"
#include "SVF-FE/SVFIRBuilder.h"
#include "WPA/Andersen.h"

using namespace SVF;
using namespace SVFUtil;

/// Initialize analysis
void UseDefAnalysis::initialize(SVFModule *M) {
  SVFIRBuilder Builder;
  SVFIR *Pag = Builder.build(M);

  AndersenWaveDiff *Ander = AndersenWaveDiff::createAndersenWaveDiff(Pag);
  Svfg = SvfgBuilder.buildFullSVFG(Ander);
}

/// Start analysis.
void UseDefAnalysis::analyze(SVFModule *M) {
  initialize(M);

  // TODO: analyze SVFG for Use-Def calculation.
}