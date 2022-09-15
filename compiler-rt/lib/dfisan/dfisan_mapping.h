//===-- dfisan_interface_internal.h -----------------------------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//
///
/// Defines DFISan memory mapping.
///
//===----------------------------------------------------------------------===//

// Selective DFI's memory mapping (tentative)
//  - UnsafeHeap:        size 0x8000_0000
//  - SafeAlignedHeap:   size 0x8000_0000, ShadowAligned:   size 0x4000_0000
//  - SafeUnalignedHeap: size 0x8000_0000, ShadowUnaligned: size 0x0001_0000_0000
//
// ShadowAligned = (SafeAligned >> 1)
// ShadowUnalinged = (SafeUnaligned << 1)
//
// || `[0x7000_7fff_8000, 0x7fff_ffff_ffff]` || UnsafeStack     ||
// || `[0x1000_0000_0000, 0x1000_7fff_ffff]` || UnsafeHeap      ||
// || `[0x0008_0000_0000, 0x0008_7fff_ffff]` || SafeAligned     ||
// || `[0x0004_0000_0000, 0x0004_3fff_ffff]` || ShadowAligned   ||
// || `[0x0002_0000_0000, 0x0003_ffff_ffff]` || ShadowGap       ||
// || `[0x0001_0000_0000, 0x0001_ffff_ffff]` || ShadowUnalinged ||
// || `[0x0000_8000_0000, 0x0000_ffff_ffff]` || SafeUnalinged   ||
// || `[0x0000_7fff_8000, 0x0000_7fff_ffff]` || Gap ||
// || `[0x0000_0000_0000, 0x0000_7fff_7fff]` || LowUnsafe       ||

#include "sanitizer_common/sanitizer_internal_defs.h"

using __sanitizer::u8;
using __sanitizer::u64;
using __sanitizer::uptr;

constexpr u8 kAlignedShiftWidth = 1;
constexpr u8 kUnalignedShiftWidth = 1;
constexpr u8 kAlignedMemGranularity = 4;
constexpr u8 kShadowGranularity = 2;

constexpr uptr kUnsafeStackEnd     = 0x7fffffffffff;
constexpr uptr kUnsafeStackBeg     = 0x70007fff8000;
constexpr uptr kUnsafeHeapEnd      = 0x10007fffffff;
constexpr uptr kUnsafeHeapBeg      = 0x100000000000;
constexpr uptr kSafeAlignedEnd     = 0x87fffffff;
constexpr uptr kSafeAlignedBeg     = 0x800000000;
constexpr uptr kShadowAlignedEnd   = 0x43fffffff;
constexpr uptr kShadowAlignedBeg   = 0x400000000;
constexpr uptr kShadowGapEnd       = 0x3ffffffff;
constexpr uptr kShadowGapBeg       = 0x200000000;
constexpr uptr kShadowUnalignedEnd = 0x1ffffffff;
constexpr uptr kShadowUnalignedBeg = 0x100000000;
constexpr uptr kSafeUnalignedEnd   = 0xffffffff;
constexpr uptr kSafeUnalignedBeg   = 0x80000000;
constexpr uptr kLowUnsafeEnd       = 0x7fff7fff;
constexpr uptr kLowUnsafeBeg       = 0x0;

namespace __dfisan {

static inline uptr AlignAddr(uptr addr) {
  return addr & ~(kAlignedMemGranularity - 1);   // Clear Lower 2 bit for 4 bytes alignment
}

static inline bool AddrIsInUnsafeHeap(uptr addr) {
  return kUnsafeHeapBeg <= addr && addr <= kUnsafeHeapEnd;
}

static inline bool AddrIsInSafeAligned(uptr addr) {
  return kSafeAlignedBeg <= addr && addr <= kSafeAlignedEnd;
}

static inline bool AddrIsInSafeUnaligned(uptr addr) {
  return kSafeUnalignedBeg <= addr && addr <= kSafeUnalignedEnd;
}

static inline bool AddrIsInShadowAligned(uptr addr) {
  return kShadowAlignedBeg <= addr && addr <= kShadowAlignedEnd;
}

static inline bool AddrIsInShadowUnaligned(uptr addr) {
  return kShadowUnalignedBeg <= addr && addr <= kShadowUnalignedEnd;
}

static inline bool AddrIsInShadowGap(uptr addr) {
  return kShadowGapBeg <= addr && addr <= kShadowGapEnd;
}

static inline bool AddrIsInUnsafeRegion(uptr addr) {
  return (kUnsafeStackBeg <= addr && addr <= kUnsafeStackEnd) ||
         (kLowUnsafeBeg <= addr && addr <= kLowUnsafeEnd)     ||
         AddrIsInUnsafeHeap(addr);
}

static inline bool AddrIsInSafeRegion(uptr addr) {
  return AddrIsInSafeAligned(addr) || AddrIsInSafeUnaligned(addr);
}

static inline bool AddrIsInShadow(uptr addr) {
  return AddrIsInShadowAligned(addr) || AddrIsInShadowUnaligned(addr);
}

static inline uptr AlignedMemToShadow(uptr addr) {
  CHECK(AddrIsInSafeAligned(addr));
  return (addr >> kAlignedShiftWidth);
}

static inline uptr UnalignedMemToShadow(uptr addr) {
  CHECK(AddrIsInSafeUnaligned(addr));
  return (addr << kUnalignedShiftWidth);
}

// Same as ceil(size, 4)
static inline u64 getShadowAlignedSize(u64 size) {
  return (size / kAlignedMemGranularity) + ((size % kAlignedMemGranularity) != 0);
}

} // namespace __dfisan

/*

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
*/
