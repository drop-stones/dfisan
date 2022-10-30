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

#include "dfisan_interface_internal.h"
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
  constexpr uptr kSafeAlignedHeapEnd   = kSafeAlignedEnd;
  constexpr uptr kSafeAlignedHeapBeg   = 0x810000000;
  constexpr uptr kSafeAlignedGlobalEnd = 0x80fffffff;
  constexpr uptr kSafeAlignedGlobalBeg = kSafeAlignedBeg;
constexpr uptr kShadowAlignedEnd   = 0x43fffffff;
constexpr uptr kShadowAlignedBeg   = 0x400000000;
constexpr uptr kShadowGapEnd       = 0x3ffffffff;
constexpr uptr kShadowGapBeg       = 0x200000000;
constexpr uptr kShadowUnalignedEnd = 0x1ffffffff;
constexpr uptr kShadowUnalignedBeg = 0x100000000;
constexpr uptr kSafeUnalignedEnd   = 0xffffffff;
constexpr uptr kSafeUnalignedBeg   = 0x80000000;
  constexpr uptr kSafeUnalignedHeapEnd   = kSafeUnalignedEnd;
  constexpr uptr kSafeUnalignedHeapBeg   = 0x90000000;
  constexpr uptr kSafeUnalignedGlobalEnd = 0x8fffffff;
  constexpr uptr kSafeUnalignedGlobalBeg = kSafeUnalignedBeg;
constexpr uptr kLowUnsafeEnd       = 0x7fff7fff;
constexpr uptr kLowUnsafeBeg       = 0x0;

namespace __dfisan {

static inline uptr AlignAddr(uptr addr) {
  return addr & ~(kAlignedMemGranularity - 1);   // Clear Lower 2 bit for 4 bytes alignment
}

static inline bool AddrIsInUnsafeHeap(uptr addr) {
  return kUnsafeHeapBeg <= addr && addr <= kUnsafeHeapEnd;
}

static inline bool AddrIsInSafeAlignedHeap(uptr addr) {
  return kSafeAlignedHeapBeg <= addr && addr <= kSafeAlignedHeapEnd;
}

static inline bool AddrIsInSafeAlignedGlobal(uptr addr) {
  return kSafeAlignedGlobalBeg <= addr && addr <= kSafeAlignedGlobalEnd;
}

static inline bool AddrIsInSafeUnalignedHeap(uptr addr) {
  return kSafeUnalignedHeapBeg <= addr && addr <= kSafeUnalignedHeapEnd;
}

static inline bool AddrIsInSafeUnalignedGlobal(uptr addr) {
  return kSafeUnalignedGlobalBeg <= addr && addr <= kSafeUnalignedGlobalEnd;
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

static inline bool AddrIsInSafeAlignedRegion(uptr addr) {
  return kSafeAlignedBeg <= addr && addr <= kSafeAlignedEnd;
}

static inline bool AddrIsInSafeUnalignedRegion(uptr addr) {
  return kSafeUnalignedBeg <= addr && addr <= kSafeUnalignedEnd;
}

static inline bool AddrIsInSafeRegion(uptr addr) {
  return AddrIsInSafeAlignedRegion(addr) || AddrIsInSafeUnalignedRegion(addr);
}

static inline bool AddrIsInShadow(uptr addr) {
  return AddrIsInShadowAligned(addr) || AddrIsInShadowUnaligned(addr);
}

static inline uptr AlignedMemToShadow(uptr addr) {
  // CHECK(AddrIsInSafeAlignedRegion(addr));
  if (!AddrIsInSafeAlignedRegion(addr))
    __dfisan_invalid_safe_access_report(addr);
  return (addr >> kAlignedShiftWidth);
}

static inline uptr UnalignedMemToShadow(uptr addr) {
  // CHECK(AddrIsInSafeUnalignedRegion(addr));
  if (!AddrIsInSafeUnalignedRegion(addr))
    __dfisan_invalid_safe_access_report(addr);
  return (addr << kUnalignedShiftWidth);
}

// Same as ceil(size, 4)
static inline u64 getShadowAlignedSize(u64 size) {
  return (size / kAlignedMemGranularity) + ((size % kAlignedMemGranularity) != 0);
}

} // namespace __dfisan
