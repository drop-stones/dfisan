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
    dlfree(ptr);
  }
  return;
}

INTERCEPTOR(void *, calloc, SIZE_T size, SIZE_T elem_size) {
  void *ptr = __dfisan_unsafe_calloc(size, elem_size);
  return ptr;
}

INTERCEPTOR(void *, realloc, void *ptr, SIZE_T size) {
  void *new_ptr = __dfisan_unsafe_realloc(ptr, size);
  return new_ptr;
}

INTERCEPTOR(void *, mmap, void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
  void *ptr = __dfisan_unsafe_mmap(addr, length, prot, flags, fd, offset);
  return ptr;
}

namespace __dfisan {

static bool interceptors_initialized = false;

void InitializeDfisanInterceptors() {
  Report("Initialize interceptors...\n");
  CHECK((interceptors_initialized == false));

  INTERCEPT_FUNCTION(malloc);
  INTERCEPT_FUNCTION(free);
  INTERCEPT_FUNCTION(calloc);
  INTERCEPT_FUNCTION(realloc);
  INTERCEPT_FUNCTION(mmap);

  interceptors_initialized = true;
}
} // namespace __dfisan