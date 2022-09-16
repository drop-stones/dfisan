//===-- dfisan_interceptors.cpp ---------------------------------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//
///
/// Intercept various libc functions and replace it with wrapper functions.
///
//===----------------------------------------------------------------------===//
#include "dfisan/dfisan_interceptors.h"
#include "dfisan/dfisan_malloc.h"
#include "dfisan/dfisan_mapping.h"

#include "interception/interception.h"
#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_internal_defs.h"

#include <assert.h>

using namespace __sanitizer;

INTERCEPTOR(void *, malloc, SIZE_T size) {
  // Report("%s: Intercept malloc(%lu), address(%p)\n", __func__, (unsigned long)size, ptr);
  void *ptr = __dfisan_unsafe_malloc(size);
  return ptr;
}

INTERCEPTOR(void, free, void *ptr) {
  // Report("%s: Intercept free(%p)\n", __func__, ptr);
  if (__dfisan::AddrIsInUnsafeHeap((uptr)ptr)) {
    __dfisan_unsafe_free(ptr);
  } else if (__dfisan::AddrIsInSafeAligned((uptr)ptr)) {
    __dfisan_safe_aligned_free(ptr);
  } else if (__dfisan::AddrIsInSafeUnaligned((uptr)ptr)) {
    __dfisan_safe_unaligned_free(ptr);
  } else {
    assert(false && "Invalid address of free");
  }
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