//===-- dfisan_interface_internal.h -----------------------------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//
///
/// DFISan-private header which defines various general utilities.
///
//===----------------------------------------------------------------------===//
#ifndef DFISAN_INTERNAL_H
#define DFISAN_INTERNAL_H

namespace __dfisan {

// dfisan_shadow_setup.cpp
void InitializeShadowMemory();

/// Statistics
void CountUnsafeAccess();
void CountAlignedStore();
void CountUnalignedStore();
void CountAlignedLoad();
void CountUnalignedLoad();
void PrintStatistics();

}  // namespace __dfisan

#endif