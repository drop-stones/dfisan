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
using __sanitizer::u32;
using __sanitizer::u16;

extern "C" {
  // This function should be called at the very beginning of the process,
  // before any instrumented code is executed and before any call to malloc.
  SANITIZER_INTERFACE_ATTRIBUTE void __dfisan_init();

  // Sets Def-IDs to the given range of the shadow memory.
  SANITIZER_INTERFACE_ATTRIBUTE void __dfisan_store_id   (uptr StoreAddr, u16 DefID);
  SANITIZER_INTERFACE_ATTRIBUTE void __dfisan_store_id_1 (uptr StoreAddr, u16 DefID);
  SANITIZER_INTERFACE_ATTRIBUTE void __dfisan_store_id_2 (uptr StoreAddr, u16 DefID);
  SANITIZER_INTERFACE_ATTRIBUTE void __dfisan_store_id_4 (uptr StoreAddr, u16 DefID);
  SANITIZER_INTERFACE_ATTRIBUTE void __dfisan_store_id_8 (uptr StoreAddr, u16 DefID);
  SANITIZER_INTERFACE_ATTRIBUTE void __dfisan_store_id_16(uptr StoreAddr, u16 DefID);

  // Checks whether Def-IDs in the given range of the shadow memory are the same as correct IDs.
  SANITIZER_INTERFACE_ATTRIBUTE void __dfisan_check_ids   (uptr LoadAddr, u16 Argc, ...);
  SANITIZER_INTERFACE_ATTRIBUTE void __dfisan_check_ids_1 (uptr LoadAddr, u16 Argc, ...);
  SANITIZER_INTERFACE_ATTRIBUTE void __dfisan_check_ids_2 (uptr LoadAddr, u16 Argc, ...);
  SANITIZER_INTERFACE_ATTRIBUTE void __dfisan_check_ids_4 (uptr LoadAddr, u16 Argc, ...);
  SANITIZER_INTERFACE_ATTRIBUTE void __dfisan_check_ids_8 (uptr LoadAddr, u16 Argc, ...);
  SANITIZER_INTERFACE_ATTRIBUTE void __dfisan_check_ids_16(uptr LoadAddr, u16 Argc, ...);

} // externc "C"

#endif