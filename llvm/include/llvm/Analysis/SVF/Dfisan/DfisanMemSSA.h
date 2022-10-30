#ifndef DFISAN_MEMSSA_H_
#define DFISAN_MEMSSA_H_

#include "MSSA/MemSSA.h"

namespace SVF {

class DfisanMemSSA : public MemSSA {
public:
  /// Constructor
  DfisanMemSSA(BVDataPTAImpl *p, bool ptrOnlyMSSA) : MemSSA(p, ptrOnlyMSSA) {}

  /// Destructor
  virtual ~DfisanMemSSA() {}

protected:
  virtual void createMUCHI(const SVFFunction &Fun) override;
};

} // namespace SVF

#endif
