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
#include "dfisan/dfisan_errors.h"

#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_internal_defs.h"
#include "sanitizer_common/sanitizer_stacktrace.h"

#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <math.h>

using namespace __sanitizer;

namespace __dfisan {

bool dfisan_inited = false;
bool dfisan_init_is_running = false;

bool dfisan_statistics = false;
u64 NumUnsafeAccesses  = 0;
u64 NumAlignedStores   = 0;
u64 NumUnalignedStores = 0;
u64 NumAlignedLoads    = 0;
u64 NumUnalignedLoads  = 0;

/* --- Error report --- */
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_invalid_safe_access_report(uptr Addr) {
  GET_CALLER_PC_BP;
  Decorator d;
  Printf("\n%s", d.Error());
  Printf("ERROR: Invalid access to safe region (%p)\n", (void *)Addr);
  Printf("%s", d.Default());

  Printf("\n%s", d.StackTrace());
  Printf("StackTrace:\n");
  Printf("%s", d.Default());
  BufferedStackTrace stack;
  stack.Unwind(pc, bp, nullptr, common_flags()->fast_unwind_on_fatal);
  stack.Print();

  exit(1);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_invalid_use_report(uptr LoadAddr, u16 Argc, ...) {
  va_list IDList;
  u16 *shadow_memory = NULL;
  if (AddrIsInSafeAlignedRegion(LoadAddr))
    shadow_memory = (u16 *)AlignedMemToShadow(LoadAddr);
  else if (AddrIsInSafeUnalignedRegion(LoadAddr))
    shadow_memory = (u16 *)UnalignedMemToShadow(LoadAddr);
  else
    assert(false && "Invalid LoadAddr");
  u16 ErrID = *shadow_memory;
  REPORT_ERROR(LoadAddr, ErrID, Argc, IDList);
}

static void DfisanInitInternal() {
  CHECK(dfisan_init_is_running == false);
  if (dfisan_inited == true)
    return;
  dfisan_init_is_running = true;

  InitializeShadowMemory();
  InitializeDfisanInterceptors();

  SetCommonFlagsDefaults();   // for Decorator

  dfisan_init_is_running = false;
  dfisan_inited = true;
}

/* --- Statistics --- */
void CountUnsafeAccess()   { dfisan_statistics = true; NumUnsafeAccesses++; }
void CountAlignedStore()   { dfisan_statistics = true; NumAlignedStores++;  }
void CountUnalignedStore() { dfisan_statistics = true; NumUnalignedStores++; }
void CountAlignedLoad()    { dfisan_statistics = true; NumAlignedLoads++; }
void CountUnalignedLoad()  { dfisan_statistics = true; NumUnalignedLoads++; }

void PrintStatistics() {
  Report("===== STATISTICS =====\n");
  Report("UnsafeAccess:  \t%lu\n", NumUnsafeAccesses);
  Report("AlignedStore:  \t%lu\n", NumAlignedStores);
  Report("UnalignedStore:\t%lu\n", NumUnalignedStores);
  Report("AlignedLoad:   \t%lu\n", NumAlignedLoads);
  Report("UnalignedLoads:\t%lu\n", NumUnalignedLoads);
  Report("======================\n");
}

} // namespace __dfisan

// ------------------ Interface ---------------------
using namespace __dfisan;

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_init() {
  Report("INFO: Initialize dfisan\n");
  DfisanInitInternal();
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_fini() {
  Report("INFO: Finish dfisan\n");
  if (dfisan_statistics)
    PrintStatistics();
}
