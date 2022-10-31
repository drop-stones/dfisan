#ifndef DFISAN_MTA_H_
#define DFISAN_MTA_H_

#include "MTA/MTA.h"

namespace SVF {

class DfisanMTA : public MTA {
public:
  /// Constructor
  DfisanMTA() : MTA() {}

  /// Destructor
  virtual ~DfisanMTA() {}

  /// Compute MHP
  virtual MHP *computeMHP(SVFModule *module) override;
};

} // namespace SVF

#endif
