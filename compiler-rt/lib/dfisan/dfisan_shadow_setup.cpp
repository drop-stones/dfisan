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

using namespace __sanitizer;

namespace __dfisan {

void InitializeShadowMemory() {
  // mmap the low shadow
  ReserveShadowMemoryRange(kLowShadowBeg, kHighShadowEnd, "low shadow");
  Report("INFO: Reserve low shadow\n");
  // mmap the high shadow
  ReserveShadowMemoryRange(kHighShadowBeg, kHighShadowEnd, "high shadow");
  Report("INFO: Reserve high shadow\n");
  // mmap the shadow gap
  ProtectGap(kShadowGapBeg, kShadowGapEnd - kShadowGapBeg + 1, 0, (1 << 18));
  Report("INFO: Reserve shadow gap\n");
  CHECK_EQ(kShadowGapEnd, kHighShadowBeg - 1);
}

} // namespace __dfisan
