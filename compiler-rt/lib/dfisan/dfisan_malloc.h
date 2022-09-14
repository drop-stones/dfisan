#include "dlmalloc/dlmalloc.h"

// globals
extern void *unsafe_heap;
extern void *safe_aligned_heap;
extern void *safe_unaligned_heap;
extern mspace unsafe_region, safe_aligned_region, safe_unalinged_region;

// mallocs
void* __dfisan_unsafe_malloc(size_t n);
void* __dfisan_safe_aligned_malloc(size_t n);
void* __dfisan_safe_unaligned_malloc(size_t n);

// frees
void __dfisan_unsafe_free(void *ptr);
void __dfisan_safe_aligned_free(void *ptr);
void __dfisan_safe_unaligned_free(void *ptr);
