//===-- dfisan_interface_internal.h -----------------------------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//
///
/// Defines DFISan memory mapping.
///
//===----------------------------------------------------------------------===//

// ShadowMem = (Mem > SHIFT_WIDTH) + SHADOW_OFFSET
//
// Shadow mapping on Linux/x86_64 with SHADOW_OFFSET == 0x7fff_8000 and SHIFT_WIDTH == 1:
// || `[0x4000_7fff_8000, 0x7fff_ffff_ffff]` || HighMem    ||
// || `[0x2000_ffff_0000, 0x4000_7fff_7fff]` || HighShadow ||
// || `[0x0000_bfff_4000, 0x2000_fffe_ffff]` || ShadowGap  ||
// || `[0x0000_7fff_8000, 0x0000_bfff_3fff]` || LowShadow  ||
// || `[0x0000_0000_0000, 0x0000_7fff_7fff]` || LowMem     ||
//
// We provides 2 bytes in shadow memory for 4 bytes in memory.

#include "sanitizer_common/sanitizer_internal_defs.h"

using __sanitizer::u8;
using __sanitizer::uptr;

constexpr uptr kShadowOffset  = 0x7fff8000;
constexpr u8   kShiftWidth    = 1;
constexpr u8   kMemGranularity    = 4;
constexpr u8   kShadowGranularity = 2;

constexpr uptr kHighMemEnd    = 0x7fffffffffff;
constexpr uptr kHighMemBeg    = 0x40007fff8000;
constexpr uptr kHighShadowEnd = 0x40007fff7fff;
constexpr uptr kHighShadowBeg = 0x2000ffff0000;
constexpr uptr kShadowGapEnd  = 0x2000fffeffff;
constexpr uptr kShadowGapBeg  = 0xbfff4000;
constexpr uptr kLowShadowEnd  = 0xbfff3fff;
constexpr uptr kLowShadowBeg  = 0x7fff8000;
constexpr uptr kLowMemEnd     = 0x7fff7fff;
constexpr uptr kLowMemBeg     = 0x0;

namespace __dfisan {

static inline uptr AlignAddr(uptr addr) {
  return addr & ~(kMemGranularity - 1);    // Clear Lower 2 bit for 4 bytes alignment
}

static inline bool AddrIsInLowMem(uptr addr) {
  return addr <= kLowMemEnd;
}

static inline bool AddrIsInHighMem(uptr addr) {
  return kHighMemBeg <= addr && addr <= kHighMemEnd;
}

static inline bool AddrIsInLowShadow(uptr addr) {
  return kLowShadowBeg <= addr && addr <= kLowShadowEnd;
}

static inline bool AddrIsInHighShadow(uptr addr) {
  return kHighShadowBeg <= addr && addr <= kHighShadowEnd;
}

static inline bool AddrIsInShadowGap(uptr addr) {
  return kShadowGapBeg <= addr && addr <= kShadowGapEnd;
}

static inline bool AddrIsInMem(uptr addr) {
  return AddrIsInLowMem(addr) || AddrIsInHighMem(addr);
}

static inline bool AddrIsInShadow(uptr addr) {
  return AddrIsInLowShadow(addr) || AddrIsInHighShadow(addr);
}

static inline uptr MemToShadow(uptr addr) {
  CHECK(AddrIsInMem(addr));
  return (addr >> kShiftWidth) + kShadowOffset;
}

} // namespace __dfisan