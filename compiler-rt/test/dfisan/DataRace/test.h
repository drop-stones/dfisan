#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
// #include "sanitizer_common/print_address.h"

// Dfisan-invisible barrier.
// Tests use it to establish necessary execution order in a way that does not
// interfere with dfisan (does not establish synchronization between threads).
typedef unsigned invisible_barrier_t;

#ifdef __cplusplus
extern "C" {
#endif
void __dfisan_testonly_barrier_init(invisible_barrier_t *barrier,
    unsigned count);
void __dfisan_testonly_barrier_wait(invisible_barrier_t *barrier);
unsigned long __dfisan_testonly_shadow_stack_current_size();
#ifdef __cplusplus
}
#endif

static inline void barrier_init(invisible_barrier_t *barrier, unsigned count) {
  __dfisan_testonly_barrier_init(barrier, count);
}

static inline void barrier_wait(invisible_barrier_t *barrier) {
  __dfisan_testonly_barrier_wait(barrier);
}

// Default instance of the barrier, but a test can declare more manually.
invisible_barrier_t barrier;
