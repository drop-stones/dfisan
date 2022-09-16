#include "dlmalloc/dlmalloc.h"
#include "dfisan_interface_internal.h"

// globals
extern void *unsafe_heap;
extern void *safe_aligned_heap;
extern void *safe_unaligned_heap;
extern mspace unsafe_region, safe_aligned_region, safe_unaligned_region;

// Initialize memory layout
void* ReserveRegion(size_t start, size_t end, const char *name);
void ReserveUnsafeRegion(size_t start, size_t end);
void ReserveSafeAlignedRegion(size_t mem_start, size_t mem_end, size_t shadow_start, size_t shadow_end);
void ReserveSafeUnalignedRegion(size_t mem_start, size_t mem_end, size_t shadow_start, size_t shadow_end);
