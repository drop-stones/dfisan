//===-- dfisan_shadow_setup.cpp ---------------------------------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//
///
/// Initialize shadow memory using mmap().
/// Please see "dfisan/dfisan_mapping.h" about memory mapping.
///
//===----------------------------------------------------------------------===//
#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_platform.h"
#include "dfisan/dfisan_internal.h"
#include "dfisan/dfisan_mapping.h"
#include "dfisan/dfisan_malloc.h"

#include <sys/mman.h>
#include <assert.h>

using namespace __sanitizer;

namespace __dfisan {

void InitializeShadowMemory() {
  // mmap the unsafe heap
  ReserveUnsafeRegion(kUnsafeHeapBeg, kUnsafeHeapEnd);
  // mmap the safe aligned heap
  ReserveSafeAlignedRegion(kSafeAlignedGlobalBeg, kSafeAlignedGlobalEnd, kSafeAlignedHeapBeg, kSafeAlignedHeapEnd, kShadowAlignedBeg, kShadowAlignedEnd);
  // mmap the safe unaligned heap
  ReserveSafeUnalignedRegion(kSafeUnalignedGlobalBeg, kSafeUnalignedGlobalEnd, kSafeUnalignedHeapBeg, kSafeUnalignedHeapEnd, kShadowUnalignedBeg, kShadowUnalignedEnd);

  // mmap the shadow gap
  ProtectGap(kShadowGapBeg, kShadowGapEnd - kShadowGapBeg + 1, 0, (1 << 18));
  Report("INFO: Reserve shadow gap (0x%zx, 0x%zx)\n", kShadowGapBeg, kShadowGapEnd);
}

} // namespace __dfisan
