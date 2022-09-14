#include "dfisan_malloc.h"

// __attribute__((visibility("default")))
void* __dfisan_unsafe_malloc(size_t n) {
  // return dlmalloc(n);
  return mspace_malloc(unsafe_region, n);
}

void* __dfisan_safe_aligned_malloc(size_t n) {
  return dlmalloc(n);
  // return mspace_malloc(unsafe_region, n);
}

void* __dfisan_safe_unaligned_malloc(size_t n) {
  return dlmalloc(n);
  // return mspace_malloc(unsafe_region, n);
}

void __dfisan_unsafe_free(void *ptr) {
  // dlfree(ptr);
  mspace_free(unsafe_region, ptr);
}

void __dfisan_safe_aligned_free(void *ptr) {
  dlfree(ptr);
}

void __dfisan_safe_unaligned_free(void *ptr) {
  dlfree(ptr);
}
