#ifndef DFISAN_ERRORS_H
#define DFISAN_ERRORS_H

#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_report_decorator.h"

#include <stdarg.h>

namespace __dfisan {

void ReportError(__sanitizer::uptr Addr);
void ReportInvalidUseError(__sanitizer::uptr LoadAddr, __sanitizer::u16 Argc, va_list IDList);

class Decorator : public __sanitizer::SanitizerCommonDecorator {
public:
  explicit Decorator() : SanitizerCommonDecorator() {}
  const char *Store() { return Blue(); }
  const char *Load()  { return Green(); }
};

} // namespace __dfisan

#endif