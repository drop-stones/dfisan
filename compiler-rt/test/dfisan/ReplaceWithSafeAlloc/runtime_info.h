#ifndef RUNTIME_INFO_H
#define RUNTIME_INFO_H

#include <stdlib.h>

/// Safe alloc functions
void *safe_malloc(size_t size);
void *safe_calloc(size_t nmemb, size_t size);
void *safe_realloc(void *ptr, size_t size);


/// Copy from "compiler-rt/lib/dfisan/dfisan_mapping.h"
const char kAlignedShiftWidth = 1;
const char kUnalignedShiftWidth = 1;
const char kAlignedMemGranularity = 4;
const char kShadowGranularity = 2;

const unsigned long kUnsafeStackEnd     = 0x7fffffffffff;
const unsigned long kUnsafeStackBeg     = 0x70007fff8000;
const unsigned long kUnsafeHeapEnd      = 0x10007fffffff;
const unsigned long kUnsafeHeapBeg      = 0x100000000000;
const unsigned long kSafeAlignedEnd     = 0x87fffffff;
const unsigned long kSafeAlignedBeg     = 0x800000000;
  const unsigned long kSafeAlignedHeapEnd   = kSafeAlignedEnd;
  const unsigned long kSafeAlignedHeapBeg   = 0x810000000;
  const unsigned long kSafeAlignedGlobalEnd = 0x80fffffff;
  const unsigned long kSafeAlignedGlobalBeg = kSafeAlignedBeg;
const unsigned long kShadowAlignedEnd   = 0x43fffffff;
const unsigned long kShadowAlignedBeg   = 0x400000000;
const unsigned long kShadowGapEnd       = 0x3ffffffff;
const unsigned long kShadowGapBeg       = 0x200000000;
const unsigned long kShadowUnalignedEnd = 0x1ffffffff;
const unsigned long kShadowUnalignedBeg = 0x100000000;
const unsigned long kSafeUnalignedEnd   = 0xffffffff;
const unsigned long kSafeUnalignedBeg   = 0x80000000;
  const unsigned long kSafeUnalignedHeapEnd   = kSafeUnalignedEnd;
  const unsigned long kSafeUnalignedHeapBeg   = 0x90000000;
  const unsigned long kSafeUnalignedGlobalEnd = 0x8fffffff;
  const unsigned long kSafeUnalignedGlobalBeg = kSafeUnalignedBeg;
const unsigned long kLowUnsafeEnd       = 0x7fff7fff;
const unsigned long kLowUnsafeBeg       = 0x0;

static inline unsigned long AlignAddr(unsigned long addr) {
  return addr & ~(kAlignedMemGranularity - 1);   // Clear Lower 2 bit for 4 bytes alignment
}

static inline char AddrIsInUnsafeHeap(unsigned long addr) {
  return kUnsafeHeapBeg <= addr && addr <= kUnsafeHeapEnd;
}

static inline char AddrIsInSafeAlignedHeap(unsigned long addr) {
  return kSafeAlignedHeapBeg <= addr && addr <= kSafeAlignedHeapEnd;
}

static inline char AddrIsInSafeAlignedGlobal(unsigned long addr) {
  return kSafeAlignedGlobalBeg <= addr && addr <= kSafeAlignedGlobalEnd;
}

static inline char AddrIsInSafeUnalignedHeap(unsigned long addr) {
  return kSafeUnalignedHeapBeg <= addr && addr <= kSafeUnalignedHeapEnd;
}

static inline char AddrIsInSafeUnalignedGlobal(unsigned long addr) {
  return kSafeUnalignedGlobalBeg <= addr && addr <= kSafeUnalignedGlobalEnd;
}

static inline char AddrIsInShadowAligned(unsigned long addr) {
  return kShadowAlignedBeg <= addr && addr <= kShadowAlignedEnd;
}

static inline char AddrIsInShadowUnaligned(unsigned long addr) {
  return kShadowUnalignedBeg <= addr && addr <= kShadowUnalignedEnd;
}

static inline char AddrIsInShadowGap(unsigned long addr) {
  return kShadowGapBeg <= addr && addr <= kShadowGapEnd;
}

static inline char AddrIsInUnsafeRegion(unsigned long addr) {
  return (kUnsafeStackBeg <= addr && addr <= kUnsafeStackEnd) ||
         (kLowUnsafeBeg <= addr && addr <= kLowUnsafeEnd)     ||
         AddrIsInUnsafeHeap(addr);
}

static inline char AddrIsInSafeAlignedRegion(unsigned long addr) {
  return kSafeAlignedBeg <= addr && addr <= kSafeAlignedEnd;
}

static inline char AddrIsInSafeUnalignedRegion(unsigned long addr) {
  return kSafeUnalignedBeg <= addr && addr <= kSafeUnalignedEnd;
}

static inline char AddrIsInSafeRegion(unsigned long addr) {
  return AddrIsInSafeAlignedRegion(addr) || AddrIsInSafeUnalignedRegion(addr);
}

static inline char AddrIsInShadow(unsigned long addr) {
  return AddrIsInShadowAligned(addr) || AddrIsInShadowUnaligned(addr);
}

#endif
