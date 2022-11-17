// RUN: ! %clang_dfisan %s -o %t
//
// REQUIRES: x86_64-target-arch

// ReplaceWithSafeAlloc cannot replace malloc wrapper because it cannot decide allocated place (Aligned or Unaligned region).

#include "../safe_alloc.h"

void *malloc_wrapper(size_t size) {
  return safe_malloc(size);
}

int main(void) {
  int *arr = (int *)malloc_wrapper(sizeof(int) * 100);
  return 0;
}
