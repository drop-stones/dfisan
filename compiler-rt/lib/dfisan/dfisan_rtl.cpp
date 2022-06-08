//===-- dfisan_rtl.cpp ------------------------------------------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//
///
/// Main file of the DFISan run-time library contains the runtime functions
/// called in instrumented codes at runtime.
///
//===----------------------------------------------------------------------===//
#include "dfisan/dfisan_interceptors.h"
#include "dfisan/dfisan_interface_internal.h"
#include "dfisan/dfisan_internal.h"
#include "dfisan/dfisan_mapping.h"

#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_internal_defs.h"

#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

using namespace __sanitizer;

// ------------- Runtime Definition Table Management -----------
static inline void setRDT(uptr Addr, u16 ID) {
  u16 *shadow_memory = (u16 *)__dfisan::MemToShadow(Addr);
  *shadow_memory = ID;
  Report("SET: %d at %p\n", ID, (void *)shadow_memory);
}
static inline void setRDT(uptr Addr, u16 ID, u8 Length) {
  u16 *shadow_memory = (u16 *)__dfisan::MemToShadow(Addr);
  for (u8 i = 0; i < Length; i++)
    *(shadow_memory + i) = ID;
  Report("SET: %d at %p - %p\n", ID, (void *)shadow_memory, (void *)(shadow_memory + Length - 1));
}
static inline bool checkRDT(uptr Addr, u16 ID) {
  u16 *shadow_memory = (u16 *)__dfisan::MemToShadow(Addr);
  Report("CHECK: %d == %d at %p\n", ID, *shadow_memory, (void *)shadow_memory);
  return *shadow_memory == ID;
}
static inline bool checkRDT(uptr Addr, u16 Argc, va_list IDList) {
  bool NoErr = false;
  for (u16 i = 0; i < Argc; i++) {
    u16 ID = va_arg(IDList, u16);
    NoErr |= checkRDT(Addr, ID);
  }
  if (NoErr == false) {
    Report("Error occured!!\n");
    exit(1);
  }
  return NoErr;
}


// ------------- Runtime check ---------------------
namespace __dfisan {

bool dfisan_inited = false;
bool dfisan_init_is_running = false;

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_store_id_n(uptr StoreAddr, u32 Size, u16 DefID) {
  //Report("INFO: Set DefID(%d) at %p\n", DefID, (void *)StoreAddr);
  setRDT(StoreAddr, DefID, ceil((double)Size / (double)4));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_store_id_1 (uptr StoreAddr, u16 DefID) {
  uptr AlignedAddr = AlignAddr(StoreAddr);
  setRDT(AlignedAddr, DefID);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_store_id_2 (uptr StoreAddr, u16 DefID) {
  uptr AlignedAddr = AlignAddr(StoreAddr);
  setRDT(AlignedAddr, DefID);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_store_id_4 (uptr StoreAddr, u16 DefID) {
  setRDT(StoreAddr, DefID);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_store_id_8 (uptr StoreAddr, u16 DefID) {
  setRDT(StoreAddr, DefID, 2);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_store_id_16(uptr StoreAddr, u16 DefID) {
  setRDT(StoreAddr, DefID, 4);
}

// TODO: va_arg cannot use `u16` (these values are converted to i32)
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_check_ids_n(uptr LoadAddr, u32 Size, u16 Argc, ...) {
  va_list IDList;
  for (u8 i = 0; i < (u8)ceil((double)Size / (double)4); i++) {
    va_start(IDList, Argc);
    checkRDT(LoadAddr + (i * 4), Argc, IDList);
  }
  va_end(IDList);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_check_ids_1 (uptr LoadAddr, u16 Argc, ...) {
  uptr AlignedAddr = AlignAddr(LoadAddr);
  va_list IDList;
  va_start(IDList, Argc);
  checkRDT(AlignedAddr, Argc, IDList);
  va_end(IDList);
}
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_check_ids_2 (uptr LoadAddr, u16 Argc, ...) {
  uptr AlignedAddr = AlignAddr(LoadAddr);
  va_list IDList;
  va_start(IDList, Argc);
  checkRDT(AlignedAddr, Argc, IDList);
  va_end(IDList);
}
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_check_ids_4 (uptr LoadAddr, u16 Argc, ...) {
  va_list IDList;
  va_start(IDList, Argc);
  checkRDT(LoadAddr, Argc, IDList);
  va_end(IDList);
}
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_check_ids_8 (uptr LoadAddr, u16 Argc, ...) {
  va_list IDList;
  va_start(IDList, Argc);
  checkRDT(LoadAddr, Argc, IDList);
  va_start(IDList, Argc);
  checkRDT(LoadAddr + 4, Argc, IDList);
  va_end(IDList);
}
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_check_ids_16(uptr LoadAddr, u16 Argc, ...) {
  va_list IDList;
  for (u8 i = 0; i < 4; i++) {
    va_start(IDList, Argc);
    checkRDT(LoadAddr + (i * 4), Argc, IDList);
  }
  va_end(IDList);
}

static void DfisanInitInternal() {
  CHECK(dfisan_init_is_running == false);
  if (dfisan_inited == true)
    return;
  dfisan_init_is_running = true;

  InitializeDfisanInterceptors();
  InitializeShadowMemory();

  dfisan_init_is_running = false;
  dfisan_inited = true;
}

} // namespace __dfisan

// ------------------ Interface ---------------------
using namespace __dfisan;

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_init() {
  Report("INFO: Initialize dfisan\n");
  DfisanInitInternal();
}
