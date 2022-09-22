#include "dlmalloc/dlmalloc.h"
#include "dfisan_interface_internal.h"

// globals
extern void *unsafe_heap;
extern void *safe_aligned_heap, *safe_aligned_glob;
extern void *safe_unaligned_heap, *safe_unaligned_glob;
extern mspace unsafe_region, safe_aligned_region, safe_unaligned_region;

// Initialize memory layout
void* ReserveRegion(size_t beg, size_t end, const char *name);
void ReserveUnsafeRegion(size_t beg, size_t end);
void ReserveSafeAlignedRegion(size_t glob_beg, size_t glob_end, size_t heap_beg, size_t heap_end, size_t shadow_start, size_t shadow_end);
void ReserveSafeUnalignedRegion(size_t glob_beg, size_t glob_end, size_t heap_beg, size_t heap_end, size_t shadow_start, size_t shadow_end);
