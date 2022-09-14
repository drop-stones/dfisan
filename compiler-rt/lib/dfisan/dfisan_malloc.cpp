#include "dfisan_malloc.h"

// __attribute__((visibility("default")))
void* __dfisan_unsafe_malloc(size_t n) {
  return dlmalloc(n);
}

void* __dfisan_safe_aligned_malloc(size_t n) {
  return dlmalloc(n);
}

void* __dfisan_safe_unaligned_malloc(size_t n) {
  return dlmalloc(n);
}
