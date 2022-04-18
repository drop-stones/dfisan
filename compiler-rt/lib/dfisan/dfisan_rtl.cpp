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
static inline void setRDT(uptr Addr, u32 ID) {
  u16 *shadow_memory = (u16 *)__dfisan::MemToShadow(Addr);
  *shadow_memory = ID;
  Report("SET: %d at %p\n", ID, (void *)shadow_memory);
}
static inline bool checkRDT(uptr Addr, u32 ID) {
  u16 *shadow_memory = (u16 *)__dfisan::MemToShadow(Addr);
  Report("CHECK: %d at %p\n", ID, (void *)shadow_memory);
  return *shadow_memory == ID;
}

// ------------- Runtime check ---------------------
namespace __dfisan {

bool dfisan_inited = false;
bool dfisan_init_is_running = false;

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_store_id(uptr StoreAddr, u32 DefID) {
  Report("INFO: Set DefID(%d) at %p\n", DefID, (void *)StoreAddr);
  setRDT(StoreAddr, DefID);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_check_ids(uptr LoadAddr, u32 Argc, ...) {
  Report("INFO: Check SetID at %p\n", (void *)LoadAddr);
  va_list Args;
  va_start(Args, Argc);
  bool NoErr = false;
  for (u32 i = 0; i < Argc; i++) {
    u32 ID = va_arg(Args, u32);
    Report("Checking SetID(%d)...\n", ID);
    NoErr |= checkRDT(LoadAddr, ID);
  }
  va_end(Args);
  if (NoErr == false) {
    Report("Error detected!!\n");
    exit(1);
  }
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
