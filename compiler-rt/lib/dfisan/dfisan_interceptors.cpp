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
#include <pthread.h>

using namespace __sanitizer;
pthread_mutex_t malloc_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t free_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t calloc_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t realloc_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mmap_lock = PTHREAD_MUTEX_INITIALIZER;

INTERCEPTOR(void *, malloc, SIZE_T size) {
  pthread_mutex_lock(&malloc_lock);
  // Report("%s: Intercept malloc(%lu), address(%p)\n", __func__, (unsigned long)size, ptr);
  void *ptr = __dfisan_unsafe_malloc(size);
  pthread_mutex_unlock(&malloc_lock);
  return ptr;
}

INTERCEPTOR(void, free, void *ptr) {
  pthread_mutex_lock(&free_lock);
  // Report("%s: Intercept free(%p)\n", __func__, ptr);
  if (__dfisan::AddrIsInUnsafeHeap((uptr)ptr)) {
    __dfisan_unsafe_free(ptr);
  } else if (__dfisan::AddrIsInSafeAlignedHeap((uptr)ptr)) {
    __dfisan_safe_aligned_free(ptr);
  } else if (__dfisan::AddrIsInSafeUnalignedHeap((uptr)ptr)) {
    __dfisan_safe_unaligned_free(ptr);
  } else {
    dlfree(ptr);
  }
  pthread_mutex_unlock(&free_lock);
  return;
}

INTERCEPTOR(void *, calloc, SIZE_T size, SIZE_T elem_size) {
  pthread_mutex_lock(&calloc_lock);
  void *ptr = __dfisan_unsafe_calloc(size, elem_size);
  pthread_mutex_unlock(&calloc_lock);
  return ptr;
}

INTERCEPTOR(void *, realloc, void *ptr, SIZE_T size) {
  pthread_mutex_lock(&realloc_lock);
  void *new_ptr = __dfisan_unsafe_realloc(ptr, size);
  pthread_mutex_unlock(&realloc_lock);
  return new_ptr;
}

INTERCEPTOR(void *, mmap, void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
  pthread_mutex_lock(&mmap_lock);
  // void *ptr = __dfisan_unsafe_mmap(addr, length, prot, flags, fd, offset);
  void *ptr = REAL(mmap(addr, length, prot, flags, fd, offset));
  pthread_mutex_unlock(&mmap_lock);
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

  pthread_mutex_init(&malloc_lock, NULL);
  pthread_mutex_init(&free_lock, NULL);
  pthread_mutex_init(&calloc_lock, NULL);
  pthread_mutex_init(&realloc_lock, NULL);
  pthread_mutex_init(&mmap_lock, NULL);

  interceptors_initialized = true;
}
} // namespace __dfisan

/// Copied from tsan_interceptors_posix.cpp
// Invisible barrier for tests.
// There were several unsuccessful iterations for this functionality:
// 1. Initially it was implemented in user code using
//    REAL(pthread_barrier_wait). But pthread_barrier_wait is not supported on
//    MacOS. Futexes are linux-specific for this matter.
// 2. Then we switched to atomics+usleep(10). But usleep produced parasitic
//    "as-if synchronized via sleep" messages in reports which failed some
//    output tests.
// 3. Then we switched to atomics+sched_yield. But this produced tons of tsan-
//    visible events, which lead to "failed to restore stack trace" failures.
// Note that no_sanitize_thread attribute does not turn off atomic interception
// so attaching it to the function defined in user code does not help.
// That's why we now have what we have.
constexpr u32 kBarrierThreadBits = 10;
constexpr u32 kBarrierThreads = 1 << kBarrierThreadBits;

extern "C" SANITIZER_INTERFACE_ATTRIBUTE void __dfisan_testonly_barrier_init(
    atomic_uint32_t *barrier, u32 num_threads) {
  if (num_threads >= kBarrierThreads) {
    Printf("barrier_init: count is too large (%d)\n", num_threads);
    Die();
  }
  // kBarrierThreadBits lsb is thread count,
  // the remaining are count of entered threads.
  atomic_store(barrier, num_threads, memory_order_relaxed);
}

static u32 barrier_epoch(u32 value) {
  return (value >> kBarrierThreadBits) / (value & (kBarrierThreads - 1));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE void __dfisan_testonly_barrier_wait(
    atomic_uint32_t *barrier) {
  u32 old = atomic_fetch_add(barrier, kBarrierThreads, memory_order_relaxed);
  u32 old_epoch = barrier_epoch(old);
  if (barrier_epoch(old + kBarrierThreads) != old_epoch) {
    FutexWake(barrier, (1 << 30));
    return;
  }
  for (;;) {
    u32 cur = atomic_load(barrier, memory_order_relaxed);
    if (barrier_epoch(cur) != old_epoch)
      return;
    FutexWait(barrier, cur);
  }
}
