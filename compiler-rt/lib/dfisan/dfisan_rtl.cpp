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
#include <math.h>

using namespace __sanitizer;

// ------------- Runtime Definition Table Management -----------
// --- Aligned RDT ---
// Set DefID about 1-4 bytes definition.
static inline void setAlignedRDT(uptr Addr, u16 ID) {
  u16 *shadow_memory = (u16 *)__dfisan::AlignedMemToShadow(Addr);
  *shadow_memory = ID;
}
// Set DefID about 5- bytes definition
static inline void setAlignedRDT(uptr Addr, u16 ID, u64 Length) {
  u16 *shadow_memory = (u16 *)__dfisan::AlignedMemToShadow(Addr);
  for (u64 i = 0; i < Length; i++)
    *(shadow_memory + i) = ID;
}

static inline bool checkAlignedRDT(uptr Addr, u16 ID) {
  u16 *shadow_memory = (u16 *)__dfisan::AlignedMemToShadow(Addr);
  return *shadow_memory == ID;
}
static inline bool checkAlignedRDT(uptr Addr, u32 Argc, va_list IDList) {
  bool NoErr = false;
  for (u32 i = 0; i < Argc; i++) {
    u16 ID = (u16)va_arg(IDList, u32);
    NoErr |= checkAlignedRDT(Addr, ID);
  }
  return NoErr;
}
#define CHECK_ALIGNED_ID(LoadAddr, Argc, IDList)        \
  va_start(IDList, Argc);                               \
  if (checkAlignedRDT(LoadAddr, Argc, IDList) == false) \
    REPORT_ERROR(LoadAddr, Argc, IDList)

// --- Unaligned RDT ---
static inline void setUnalignedRDT(uptr Addr, u16 ID) {
  u16 *shadow_memory = (u16 *)__dfisan::UnalignedMemToShadow(Addr);
  *shadow_memory = ID;
}
static inline void setUnalignedRDT(uptr Addr, u16 ID, u32 Length) {
  u16 *shadow_memory = (u16 *)__dfisan::UnalignedMemToShadow(Addr);
  for (u32 i = 0; i < Length; i++)
    *(shadow_memory + i) = ID;
}

static inline bool checkUnalignedRDT(uptr Addr, u16 ID) {
  u16 *shadow_memory = (u16 *)__dfisan::UnalignedMemToShadow(Addr);
  return *shadow_memory == ID;
}
static inline bool checkUnalignedRDT(uptr Addr, u32 Argc, va_list IDList) {
  bool NoErr = false;
  for (u32 i = 0; i < Argc; i++) {
    u16 ID = (u16)va_arg(IDList, u32);
    NoErr |= checkUnalignedRDT(Addr, ID);
  }
  return NoErr;
}
#define CHECK_UNALIGNED_ID(LoadAddr, Argc, IDList)        \
  va_start(IDList, Argc);                                 \
  if (checkUnalignedRDT(LoadAddr, Argc, IDList) == false) \
    REPORT_ERROR(LoadAddr, Argc, IDList)

/*
static inline void setRDT(uptr Addr, u16 ID) {
  u16 *shadow_memory = (u16 *)__dfisan::MemToShadow(Addr);
  *shadow_memory = ID;
  // Report("SET: %d at %p\n", ID, (void *)shadow_memory);
}
static inline void setRDT(uptr Addr, u16 ID, u32 Length) {
  u16 *shadow_memory = (u16 *)__dfisan::MemToShadow(Addr);
  for (u32 i = 0; i < Length; i++)
    *(shadow_memory + i) = ID;
  // Report("SET: %d at %p - %p\n", ID, (void *)shadow_memory, (void *)(shadow_memory + Length - 1));
}
static inline bool checkRDT(uptr Addr, u16 ID) {
  u16 *shadow_memory = (u16 *)__dfisan::MemToShadow(Addr);
  // Report("CHECK: %d == %d at %p\n", ID, *shadow_memory, (void *)shadow_memory);
  return *shadow_memory == ID;
}
static inline bool checkRDT(uptr Addr, u16 Argc, va_list IDList) {
  bool NoErr = false;
  for (u16 i = 0; i < Argc; i++) {
    u16 ID = va_arg(IDList, u16);
    NoErr |= checkRDT(Addr, ID);
  }
  return NoErr;
}
*/

// ------------- Runtime check ---------------------
namespace __dfisan {

bool dfisan_inited = false;
bool dfisan_init_is_running = false;

// --- Aligned Heap Region ---
// Set functions
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_store_id_n(uptr StoreAddr, u64 Size, u16 DefID) {
  setAlignedRDT(StoreAddr, DefID, getShadowAlignedSize(Size));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_store_id_1(uptr StoreAddr, u16 DefID) {
  uptr AlignedAddr = AlignAddr(StoreAddr);
  setAlignedRDT(AlignedAddr, DefID);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_store_id_2(uptr StoreAddr, u16 DefID) {
  uptr AlignedAddr = AlignAddr(StoreAddr);
  setAlignedRDT(AlignedAddr, DefID);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_store_id_4(uptr StoreAddr, u16 DefID) {
  setAlignedRDT(StoreAddr, DefID);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_store_id_8(uptr StoreAddr, u16 DefID) {
  setAlignedRDT(StoreAddr, DefID, 2);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_store_id_16(uptr StoreAddr, u16 DefID) {
  setAlignedRDT(StoreAddr, DefID, 4);
}

// Check functions
// TODO: va_arg cannot use `u16` (these values are converted to i32)
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_check_ids_n(uptr LoadAddr, u64 Size, u32 Argc, ...) {
  va_list IDList;
  for (u32 i = 0; i < getShadowAlignedSize(Size); i++) {
    CHECK_ALIGNED_ID(LoadAddr, Argc, IDList);
  }
  va_end(IDList);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_check_id_1(uptr LoadAddr, u32 Argc, ...) {
  uptr AlignedAddr = AlignAddr(LoadAddr);
  va_list IDList;
  CHECK_ALIGNED_ID(AlignedAddr, Argc, IDList);
  va_end(IDList);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_check_id_2(uptr LoadAddr, u32 Argc, ...) {
  uptr AlignedAddr = AlignAddr(LoadAddr);
  va_list IDList;
  CHECK_ALIGNED_ID(AlignedAddr, Argc, IDList);
  va_end(IDList);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_check_id_4(uptr LoadAddr, u32 Argc, ...) {
  va_list IDList;
  CHECK_ALIGNED_ID(LoadAddr, Argc, IDList);
  va_end(IDList);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_check_id_8(uptr LoadAddr, u32 Argc, ...) {
  va_list IDList;
  for (u32 i = 0; i < 2; i++) {
    CHECK_ALIGNED_ID(LoadAddr + (i * 4), Argc, IDList);
  }
  va_end(IDList);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_check_id_16(uptr LoadAddr, u32 Argc, ...) {
  va_list IDList;
  for (u32 i = 0; i < 4; i++) {
    CHECK_ALIGNED_ID(LoadAddr + (i * 4), Argc, IDList);
  }
  va_end(IDList);
}

// --- Unaligned Heap Region ---
// Set functions
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_unaligned_store_id_n(uptr StoreAddr, u64 Size, u16 DefID) {
  setUnalignedRDT(StoreAddr, DefID, Size);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_unaligned_store_id_1(uptr StoreAddr, u16 DefID) {
  setUnalignedRDT(StoreAddr, DefID);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_unaligned_store_id_2(uptr StoreAddr, u16 DefID) {
  setUnalignedRDT(StoreAddr, DefID, 2);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_unaligned_store_id_4(uptr StoreAddr, u16 DefID) {
  setUnalignedRDT(StoreAddr, DefID, 4);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_unaligned_store_id_8(uptr StoreAddr, u16 DefID) {
  setUnalignedRDT(StoreAddr, DefID, 8);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_unaligned_store_id_16(uptr StoreAddr, u16 DefID) {
  setUnalignedRDT(StoreAddr, DefID, 16);
}

// Check functions
// TODO: va_arg cannot use `u16` (these values are converted to i32)
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_unaligned_check_ids_n(uptr LoadAddr, u64 Size, u32 Argc, ...) {
  va_list IDList;
  for (u32 i = 0; i < getShadowAlignedSize(Size); i++) {
    CHECK_ALIGNED_ID(LoadAddr, Argc, IDList);
  }
  va_end(IDList);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_unaligned_check_id_1(uptr LoadAddr, u32 Argc, ...) {
  va_list IDList;
  CHECK_UNALIGNED_ID(LoadAddr, Argc, IDList);
  va_end(IDList);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_unaligned_check_id_2(uptr LoadAddr, u32 Argc, ...) {
  va_list IDList;
  for (u32 i = 0; i < 2; i++) {
    CHECK_UNALIGNED_ID(LoadAddr + i, Argc, IDList);;
  }
  va_end(IDList);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_unaligned_check_id_4(uptr LoadAddr, u32 Argc, ...) {
  va_list IDList;
  for (u32 i = 0; i < 4; i++) {
    CHECK_UNALIGNED_ID(LoadAddr + i, Argc, IDList);
  }
  va_end(IDList);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_unaligned_check_id_8(uptr LoadAddr, u32 Argc, ...) {
  va_list IDList;
  for (u32 i = 0; i < 8; i++) {
    CHECK_ALIGNED_ID(LoadAddr + i, Argc, IDList);
  }
  va_end(IDList);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_unaligned_check_id_16(uptr LoadAddr, u32 Argc, ...) {
  va_list IDList;
  for (u32 i = 0; i < 16; i++) {
    CHECK_ALIGNED_ID(LoadAddr + i, Argc, IDList);
  }
  va_end(IDList);
}

/*
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_store_id_n(uptr StoreAddr, u64 Size, u16 DefID) {
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
void __dfisan_check_ids_n(uptr LoadAddr, u64 Size, u16 Argc, ...) {
  va_list IDList;
  for (u8 i = 0; i < (u8)ceil((double)Size / (double)4); i++) {
    va_start(IDList, Argc);
    if (checkRDT(LoadAddr + (i * 4), Argc, IDList) == false) {
      REPORT_ERROR(LoadAddr, Argc, IDList);
    }
  }
  va_end(IDList);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_check_ids_1 (uptr LoadAddr, u16 Argc, ...) {
  uptr AlignedAddr = AlignAddr(LoadAddr);
  va_list IDList;
  va_start(IDList, Argc);
  if (checkRDT(AlignedAddr, Argc, IDList) == false) {
    REPORT_ERROR(LoadAddr, Argc, IDList);
  }
  va_end(IDList);
}
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_check_ids_2 (uptr LoadAddr, u16 Argc, ...) {
  uptr AlignedAddr = AlignAddr(LoadAddr);
  va_list IDList;
  va_start(IDList, Argc);
  if (checkRDT(AlignedAddr, Argc, IDList) == false) {
    REPORT_ERROR(LoadAddr, Argc, IDList);
  }
  va_end(IDList);
}
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_check_ids_4 (uptr LoadAddr, u16 Argc, ...) {
  va_list IDList;
  va_start(IDList, Argc);
  if (checkRDT(LoadAddr, Argc, IDList) == false) {
    REPORT_ERROR(LoadAddr, Argc, IDList);
  }
  va_end(IDList);
}
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_check_ids_8 (uptr LoadAddr, u16 Argc, ...) {
  va_list IDList;
  va_start(IDList, Argc);
  for (uptr i = 0; i < 2; i++) {
    va_start(IDList, Argc);
    if (checkRDT(LoadAddr + (i * 4), Argc, IDList) == false) {
      REPORT_ERROR(LoadAddr, Argc, IDList);
    }
  }
}
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_check_ids_16(uptr LoadAddr, u16 Argc, ...) {
  va_list IDList;
  for (uptr i = 0; i < 4; i++) {
    va_start(IDList, Argc);
    if (checkRDT(LoadAddr + (i * 4), Argc, IDList) == false) {
      REPORT_ERROR(LoadAddr, Argc, IDList);
    }
  }
  va_end(IDList);
}
*/

static void DfisanInitInternal() {
  CHECK(dfisan_init_is_running == false);
  if (dfisan_inited == true)
    return;
  dfisan_init_is_running = true;

  InitializeDfisanInterceptors();
  InitializeShadowMemory();

  SetCommonFlagsDefaults();   // for Decorator

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
