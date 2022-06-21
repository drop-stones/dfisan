#ifndef DFISAN_ERRORS_H
#define DFISAN_ERRORS_H

#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_report_decorator.h"

#include <stdarg.h>

#define REPORT_ERROR(LoadAddr, Argc, IDList)  \
  va_start(IDList, Argc);                     \
  GET_CALLER_PC_BP;                           \
  ReportInvalidUseError(LoadAddr, Argc, IDList, pc, bp)

namespace __dfisan {

void ReportInvalidUseError(__sanitizer::uptr LoadAddr, __sanitizer::u16 Argc, va_list IDList, __sanitizer::uptr pc, __sanitizer::uptr bp);

class Decorator : public __sanitizer::SanitizerCommonDecorator {
public:
  explicit Decorator() : SanitizerCommonDecorator() {}
  const char *Store() { return Blue(); }
  const char *Load()  { return Green(); }
};

} // namespace __dfisan

#endif