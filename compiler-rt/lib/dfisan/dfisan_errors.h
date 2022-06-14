#ifndef DFISAN_ERRORS_H
#define DFISAN_ERRORS_H

#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_report_decorator.h"

namespace __dfisan {

void ReportError(__sanitizer::uptr Addr);

class Decorator : public __sanitizer::SanitizerCommonDecorator {
public:
  Decorator() : SanitizerCommonDecorator() {}
  const char *Store() { return Blue(); }
  const char *Load()  { return Green(); }
};

} // namespace __dfisan

#endif