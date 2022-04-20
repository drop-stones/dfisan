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

using namespace __sanitizer;

// ------------- Runtime Definition Table Management -----------
static inline void setRDT(uptr Addr, u16 ID) {
  u16 *shadow_memory = (u16 *)__dfisan::MemToShadow(Addr);
  *shadow_memory = ID;
  Report("SET: %d at %p\n", ID, (void *)shadow_memory);
}
static inline bool checkRDT(uptr Addr, u16 ID) {
  u16 *shadow_memory = (u16 *)__dfisan::MemToShadow(Addr);
  Report("CHECK: %d at %p\n", ID, (void *)shadow_memory);
  return *shadow_memory == ID;
}

#define CHECK_RDT_WITH_VARARG(Addr, Argc)             \
  Report("INFO: Check SetID at %p\n", (void *)Addr);  \
  va_list Args;                                       \
  va_start(Args, Argc);                               \
  bool NoErr = false;                                 \
  for (u16 i = 0; i < Argc; i++) {                    \
    u16 ID = va_arg(Args, u16);                       \
    Report("Checking SetID(%d)...\n", ID);            \
    NoErr |= checkRDT(Addr, ID);                      \
  }                                                   \
  va_end(Args);                                       \
  if (NoErr == false) {                               \
    Report("Error detected!!\n");                     \
    exit(1);                                          \
  }


// ------------- Runtime check ---------------------
namespace __dfisan {

bool dfisan_inited = false;
bool dfisan_init_is_running = false;

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_store_id(uptr StoreAddr, u16 DefID) {
  Report("INFO: Set DefID(%d) at %p\n", DefID, (void *)StoreAddr);
  setRDT(StoreAddr, DefID);
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
  setRDT(StoreAddr, DefID);
  setRDT(StoreAddr + 4, DefID);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_store_id_16(uptr StoreAddr, u16 DefID) {
  setRDT(StoreAddr, DefID);
  setRDT(StoreAddr + 4, DefID);
  setRDT(StoreAddr + 8, DefID);
  setRDT(StoreAddr + 12, DefID);
}

// TODO: va_arg cannot use `u16` (these values are converted to i32)
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_check_ids(uptr LoadAddr, u16 Argc, ...) {
  CHECK_RDT_WITH_VARARG(LoadAddr, Argc)
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_check_ids_1 (uptr LoadAddr, u16 Argc, ...) {
  // TODO
}
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_check_ids_2 (uptr LoadAddr, u16 Argc, ...) {
  // TODO
}
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_check_ids_4 (uptr LoadAddr, u16 Argc, ...) {
  CHECK_RDT_WITH_VARARG(LoadAddr, Argc)
}
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_check_ids_8 (uptr LoadAddr, u16 Argc, ...) {
  // TODO
}
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_check_ids_16(uptr LoadAddr, u16 Argc, ...) {
  // TODO
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
