#include "dlmalloc/dlmalloc.h"

// globals
extern void *unsafe_heap;
extern void *safe_aligned_heap;
extern void *safe_unaligned_heap;
extern mspace unsafe_region, safe_aligned_region, safe_unaligned_region;

// mmaps
void* ReserveRegion(size_t start, size_t end, const char *name);
void ReserveUnsafeRegion(size_t start, size_t end);
void ReserveSafeAlignedRegion(size_t mem_start, size_t mem_end, size_t shadow_start, size_t shadow_end);
void ReserveSafeUnalignedRegion(size_t mem_start, size_t mem_end, size_t shadow_start, size_t shadow_end);

// mallocs
void* __dfisan_unsafe_malloc(size_t n);
void* __dfisan_safe_aligned_malloc(size_t n);
void* __dfisan_safe_unaligned_malloc(size_t n);

// frees
void __dfisan_unsafe_free(void *ptr);
void __dfisan_safe_aligned_free(void *ptr);
void __dfisan_safe_unaligned_free(void *ptr);
