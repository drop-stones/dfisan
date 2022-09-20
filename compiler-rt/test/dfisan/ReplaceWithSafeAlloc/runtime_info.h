#ifndef RUNTIME_INFO_H
#define RUNTIME_INFO_H

#include <stdlib.h>

/// Safe alloc functions
void *safe_malloc(size_t size);
void *safe_calloc(size_t nmemb, size_t size);
void *safe_realloc(void *ptr, size_t size);


/// Copy from "compiler-rt/lib/dfisan/dfisan_mapping.h"

char kAlignedShiftWidth = 1;
char kUnalignedShiftWidth = 1;
char kAlignedMemGranularity = 4;
char kShadowGranularity = 2;

unsigned long kUnsafeStackEnd     = 0x7fffffffffff;
unsigned long kUnsafeStackBeg     = 0x70007fff8000;
unsigned long kUnsafeHeapEnd      = 0x10007fffffff;
unsigned long kUnsafeHeapBeg      = 0x100000000000;
unsigned long kSafeAlignedEnd     = 0x87fffffff;
unsigned long kSafeAlignedBeg     = 0x800000000;
unsigned long kShadowAlignedEnd   = 0x43fffffff;
unsigned long kShadowAlignedBeg   = 0x400000000;
unsigned long kShadowGapEnd       = 0x3ffffffff;
unsigned long kShadowGapBeg       = 0x200000000;
unsigned long kShadowUnalignedEnd = 0x1ffffffff;
unsigned long kShadowUnalignedBeg = 0x100000000;
unsigned long kSafeUnalignedEnd   = 0xffffffff;
unsigned long kSafeUnalignedBeg   = 0x80000000;
unsigned long kLowUnsafeEnd       = 0x7fff7fff;
unsigned long kLowUnsafeBeg       = 0x0;

static inline unsigned long AlignAddr(unsigned long addr) {
  return addr & ~(kAlignedMemGranularity - 1);   // Clear Lower 2 bit for 4 bytes alignment
}

static inline char AddrIsInUnsafeHeap(unsigned long addr) {
  return kUnsafeHeapBeg <= addr && addr <= kUnsafeHeapEnd;
}

static inline char AddrIsInSafeAligned(unsigned long addr) {
  return kSafeAlignedBeg <= addr && addr <= kSafeAlignedEnd;
}

static inline char AddrIsInSafeUnaligned(unsigned long addr) {
  return kSafeUnalignedBeg <= addr && addr <= kSafeUnalignedEnd;
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

static inline char AddrIsInSafeRegion(unsigned long addr) {
  return AddrIsInSafeAligned(addr) || AddrIsInSafeUnaligned(addr);
}

static inline char AddrIsInShadow(unsigned long addr) {
  return AddrIsInShadowAligned(addr) || AddrIsInShadowUnaligned(addr);
}

#endif