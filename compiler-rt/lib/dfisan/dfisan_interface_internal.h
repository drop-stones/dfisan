//===-- dfisan_interface_internal.h -----------------------------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//
///
/// This header declares the DfiSanitizer runtime interface functions.
/// The runtime library has to define these functions so the instrumented program
/// could call them.
///
//===----------------------------------------------------------------------===//
#ifndef DFISAN_INTERFACE_INTERNAL_H
#define DFISAN_INTERFACE_INTERNAL_H

#include "sanitizer_common/sanitizer_internal_defs.h"

using __sanitizer::uptr;
using __sanitizer::u64;
using __sanitizer::u32;

extern "C" {
  // This function should be called at the very beginning of the process,
  // before any instrumented code is executed and before any call to malloc.
  SANITIZER_INTERFACE_ATTRIBUTE void __dfisan_init();

  // Sets Def-IDs to the given range of the shadow memory.
  SANITIZER_INTERFACE_ATTRIBUTE void __dfisan_store_id(uptr StoreAddr, u32 DefID);

  // Checks whether Def-IDs in the given range of the shadow memory are the same as correct IDs.
  SANITIZER_INTERFACE_ATTRIBUTE void __dfisan_check_ids(uptr LoadAddr, u32 Argc, ...);

} // externc "C"

#endif