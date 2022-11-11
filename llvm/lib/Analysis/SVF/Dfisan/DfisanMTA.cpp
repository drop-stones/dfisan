#include "Dfisan/DfisanMTA.h"
#include "Dfisan/DfisanSVFIRBuilder.h"
#include "WPA/Andersen.h"
#include "MTA/MHP.h"
#include "MTA/TCT.h"
#include "MTA/MTAStat.h"
#include "Util/SVFUtil.h"

using namespace SVF;
using namespace SVFUtil;

MHP *DfisanMTA::computeMHP(SVFModule *module) {
  DBOUT(DGENERAL, outs() << pasMsg("MTA analysis\n"));
  DBOUT(DMTA, outs() << pasMsg("MTA analysis\n"));
  DfisanSVFIRBuilder builder;
  SVFIR *pag = builder.build(module);
  PointerAnalysis* pta = AndersenWaveDiff::createAndersenWaveDiff(pag);

  DBOUT(DGENERAL, outs() << pasMsg("Build TCT\n"));
  DBOUT(DMTA, outs() << pasMsg("Build TCT\n"));
  DOTIMESTAT(double tctStart = stat->getClk());
  tct = new TCT(pta);
  tcg = tct->getThreadCallGraph();
  DOTIMESTAT(double tctEnd = stat->getClk());
  DOTIMESTAT(stat->TCTTime += (tctEnd - tctStart) / TIMEINTERVAL);

  if (pta->printStat())
  {
      stat->performThreadCallGraphStat(tcg);
      stat->performTCTStat(tct);
  }

  DBOUT(DGENERAL, outs() << pasMsg("MHP analysis\n"));
  DBOUT(DMTA, outs() << pasMsg("MHP analysis\n"));

  DOTIMESTAT(double mhpStart = stat->getClk());
  MHP* mhp = new MHP(tct);
  mhp->analyze();
  DOTIMESTAT(double mhpEnd = stat->getClk());
  DOTIMESTAT(stat->MHPTime += (mhpEnd - mhpStart) / TIMEINTERVAL);

  DBOUT(DGENERAL, outs() << pasMsg("MHP analysis finish\n"));
  DBOUT(DMTA, outs() << pasMsg("MHP analysis finish\n"));
  return mhp;
}
