//===-- dfisan_interceptors.cpp ---------------------------------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//
///
/// Intercept various libc functions and replace it with wrapper functions.
///
//===----------------------------------------------------------------------===//
#include "dfisan/dfisan_interceptors.h"

#include "interception/interception.h"
#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_internal_defs.h"

using namespace __sanitizer;

INTERCEPTOR(void *, malloc, SIZE_T size) {
  // Report("%s: Intercept malloc(%lu)\n", __func__, (unsigned long)size);
  void *ptr = REAL(malloc)(size);
  return ptr;
}

INTERCEPTOR(void, free, void *ptr) {
  // Report("%s: Intercept free(%p)\n", __func__, ptr);
  REAL(free(ptr));
  return;
}

namespace __dfisan {

static bool interceptors_initialized = false;

void InitializeDfisanInterceptors() {
  Report("Initialize interceptors...\n");
  CHECK((interceptors_initialized == false));

  INTERCEPT_FUNCTION(malloc);
  INTERCEPT_FUNCTION(free);

  interceptors_initialized = true;
}
} // namespace __dfisan