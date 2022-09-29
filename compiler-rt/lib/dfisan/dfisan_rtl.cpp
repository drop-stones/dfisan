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
  // Report("Set DefID(%d) at %zx\n", ID, (size_t)shadow_memory);
}
// Set DefID about 5- bytes definition
static inline void setAlignedRDT(uptr Addr, u16 ID, u64 Length) {
  u16 *shadow_memory = (u16 *)__dfisan::AlignedMemToShadow(Addr);
  for (u64 i = 0; i < Length; i++)
    *(shadow_memory + i) = ID;
  // Report("Set DefID(%d) at 0x%zx - 0x%zx\n", ID, (uptr)shadow_memory, (uptr)(shadow_memory + sizeof(u16) * Length));
}
#define SET_COND_ALIGNED_ID(StoreAddr, DefID, Size, Cond) \
  if (Cond) {                                             \
    setAlignedRDT(StoreAddr, DefID, Size);                \
  }

static inline bool checkAlignedRDT(uptr Addr, u16 ID) {
  u16 *shadow_memory = (u16 *)__dfisan::AlignedMemToShadow(Addr);
  // Report("Check RDT[0x%zx]: %d == ID: %d\n", (uptr)shadow_memory, *shadow_memory, ID);
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
#define CHECK_ALIGNED_ID(LoadAddr, Argc, IDList)          \
  va_start(IDList, Argc);                                 \
  if (checkAlignedRDT(LoadAddr, Argc, IDList) == false) { \
    REPORT_ERROR(LoadAddr, Argc, IDList);                 \
  }
#define CHECK_ALIGNED_ID_LIST(LoadAddr, Argc, Size) \
  va_list IDList;                                   \
  for (u32 i = 0; i < Size; i++) {                  \
    CHECK_ALIGNED_ID(LoadAddr, Argc, IDList);       \
  }                                                 \
  va_end(IDList)
#define CHECK_COND_ALIGNED_ID_LIST(LoadAddr, Argc, Size, Cond)  \
  if (Cond) {                                                   \
    CHECK_ALIGNED_ID_LIST(LoadAddr, Argc, Size);                \
  }

// --- Unaligned RDT ---
static inline void setUnalignedRDT(uptr Addr, u16 ID) {
  u16 *shadow_memory = (u16 *)__dfisan::UnalignedMemToShadow(Addr);
  *shadow_memory = ID;
  // Report("Set DefID(%d) at %zx\n", ID, (size_t)shadow_memory);
}
static inline void setUnalignedRDT(uptr Addr, u16 ID, u32 Length) {
  u16 *shadow_memory = (u16 *)__dfisan::UnalignedMemToShadow(Addr);
  for (u32 i = 0; i < Length; i++)
    *(shadow_memory + i) = ID;
  // Report("Set DefID(%d) at 0x%zx - 0x%zx\n", ID, (uptr)shadow_memory, (uptr)(shadow_memory + sizeof(u16) * Length));
}
#define SET_COND_UNALIGNED_ID(StoreAddr, DefID, Size, Cond) \
  if (Cond) {                                               \
    setUnalignedRDT(StoreAddr, DefID, Size);                \
  }

static inline bool checkUnalignedRDT(uptr Addr, u16 ID) {
  u16 *shadow_memory = (u16 *)__dfisan::UnalignedMemToShadow(Addr);
  // Report("Check RDT[0x%zx]: %d == ID: %d\n", (uptr)shadow_memory, *shadow_memory, ID);
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
#define CHECK_UNALIGNED_ID(LoadAddr, Argc, IDList)          \
  va_start(IDList, Argc);                                   \
  if (checkUnalignedRDT(LoadAddr, Argc, IDList) == false) { \
    REPORT_ERROR(LoadAddr, Argc, IDList);                   \
  }
#define CHECK_UNALIGNED_ID_LIST(LoadAddr, Argc, Size) \
  va_list IDList;                                     \
  for (u32 i = 0; i < Size; i++) {                    \
    CHECK_UNALIGNED_ID(LoadAddr, Argc, IDList);       \
  }                                                   \
  va_end(IDList)
#define CHECK_COND_UNALIGNED_ID_LIST(LoadAddr, Argc, Size, Cond)  \
  if (Cond) {                                                     \
    CHECK_UNALIGNED_ID_LIST(LoadAddr, Argc, Size);                \
  }

// --- Aligned or Unaligned functions ---
#define SET_ALIGNED_OR_UNALIGNED_ID(StoreAddr, DefID, Size)       \
  if (__dfisan::AddrIsInSafeAlignedRegion(StoreAddr)) {           \
    setAlignedRDT(StoreAddr, DefID, getShadowAlignedSize(Size));  \
  } else if (__dfisan::AddrIsInSafeUnalignedRegion(StoreAddr)) {  \
    setUnalignedRDT(StoreAddr, DefID, Size);                      \
  } else {                                                        \
    UNREACHABLE("Set access unsafe region!!");                    \
  }
#define SET_COND_ALIGNED_OR_UNALIGNED_ID(StoreAddr, DefID, Size)  \
  if (__dfisan::AddrIsInSafeAlignedRegion(StoreAddr)) {           \
    setAlignedRDT(StoreAddr, DefID, getShadowAlignedSize(Size));  \
  } else if (__dfisan::AddrIsInSafeUnalignedRegion(StoreAddr)) {  \
    setUnalignedRDT(StoreAddr, DefID, Size);                      \
  }

#define CHECK_ALIGNED_OR_UNALIGNED_ID_LIST(LoadAddr, Argc, Size)        \
  if (__dfisan::AddrIsInSafeAlignedRegion(LoadAddr)) {                  \
    CHECK_ALIGNED_ID_LIST(LoadAddr, Argc, getShadowAlignedSize(Size));  \
  } else if (__dfisan::AddrIsInSafeUnalignedRegion(LoadAddr)) {         \
    CHECK_UNALIGNED_ID_LIST(LoadAddr, Argc, Size);                      \
  } else {                                                              \
    UNREACHABLE("Check access unsafe region!!");                        \
  }
#define CHECK_COND_ALIGNED_OR_UNALIGNED_ID_LIST(LoadAddr, Argc, Size)   \
  if (__dfisan::AddrIsInSafeAlignedRegion(LoadAddr)) {                  \
    CHECK_ALIGNED_ID_LIST(LoadAddr, Argc, getShadowAlignedSize(Size));  \
  } else if (__dfisan::AddrIsInSafeUnalignedRegion(LoadAddr)) {         \
    CHECK_UNALIGNED_ID_LIST(LoadAddr, Argc, Size);                      \
  }

// ------------- Runtime check ---------------------
namespace __dfisan {

bool dfisan_inited = false;
bool dfisan_init_is_running = false;

/* --- Set functions --- */
// aligned
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_store_id_n(uptr StoreAddr, u64 Size, u16 DefID) {
  setAlignedRDT(StoreAddr, DefID, getShadowAlignedSize(Size));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_store_id_1(uptr StoreAddr, u16 DefID) {
  setAlignedRDT(AlignAddr(StoreAddr), DefID);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_store_id_2(uptr StoreAddr, u16 DefID) {
  setAlignedRDT(AlignAddr(StoreAddr), DefID);
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

// unaligned
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

// aligned or unaligned
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_or_unaligned_store_id_n(uptr StoreAddr, u64 Size, u16 DefID) {
  SET_ALIGNED_OR_UNALIGNED_ID(StoreAddr, DefID, Size);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_or_unaligned_store_id_1(uptr StoreAddr, u16 DefID) {
  SET_ALIGNED_OR_UNALIGNED_ID(StoreAddr, DefID, 1);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_or_unaligned_store_id_2(uptr StoreAddr, u16 DefID) {
  SET_ALIGNED_OR_UNALIGNED_ID(StoreAddr, DefID, 2);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_or_unaligned_store_id_4(uptr StoreAddr, u16 DefID) {
  SET_ALIGNED_OR_UNALIGNED_ID(StoreAddr, DefID, 4);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_or_unaligned_store_id_8(uptr StoreAddr, u16 DefID) {
  SET_ALIGNED_OR_UNALIGNED_ID(StoreAddr, DefID, 8);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_or_unaligned_store_id_16(uptr StoreAddr, u16 DefID) {
  SET_ALIGNED_OR_UNALIGNED_ID(StoreAddr, DefID, 16);
}

// conditional aligned
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_aligned_store_id_n(uptr StoreAddr, u64 Size, u16 DefID) {
  SET_COND_ALIGNED_ID(AlignAddr(StoreAddr), DefID, Size, __dfisan::AddrIsInSafeAlignedRegion(StoreAddr));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_aligned_store_id_1(uptr StoreAddr, u16 DefID) {
  SET_COND_ALIGNED_ID(AlignAddr(StoreAddr), DefID, 1, __dfisan::AddrIsInSafeAlignedRegion(StoreAddr));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_aligned_store_id_2(uptr StoreAddr, u16 DefID) {
  SET_COND_ALIGNED_ID(AlignAddr(StoreAddr), DefID, 1, __dfisan::AddrIsInSafeAlignedRegion(StoreAddr));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_aligned_store_id_4(uptr StoreAddr, u16 DefID) {
  SET_COND_ALIGNED_ID(StoreAddr, DefID, 1, __dfisan::AddrIsInSafeAlignedRegion(StoreAddr));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_aligned_store_id_8(uptr StoreAddr, u16 DefID) {
  SET_COND_ALIGNED_ID(StoreAddr, DefID, 2, __dfisan::AddrIsInSafeAlignedRegion(StoreAddr));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_aligned_store_id_16(uptr StoreAddr, u16 DefID) {
  SET_COND_ALIGNED_ID(StoreAddr, DefID, 4, __dfisan::AddrIsInSafeAlignedRegion(StoreAddr));
}

// conditional unaligned
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_unaligned_store_id_n(uptr StoreAddr, u64 Size, u16 DefID) {
  SET_COND_UNALIGNED_ID(StoreAddr, DefID, Size, __dfisan::AddrIsInSafeUnalignedRegion(StoreAddr));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_unaligned_store_id_1(uptr StoreAddr, u16 DefID) {
  SET_COND_UNALIGNED_ID(StoreAddr, DefID, 1, __dfisan::AddrIsInSafeUnalignedRegion(StoreAddr));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_unaligned_store_id_2(uptr StoreAddr, u16 DefID) {
  SET_COND_UNALIGNED_ID(StoreAddr, DefID, 2, __dfisan::AddrIsInSafeUnalignedRegion(StoreAddr));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_unaligned_store_id_4(uptr StoreAddr, u16 DefID) {
  SET_COND_UNALIGNED_ID(StoreAddr, DefID, 4, __dfisan::AddrIsInSafeUnalignedRegion(StoreAddr));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_unaligned_store_id_8(uptr StoreAddr, u16 DefID) {
  SET_COND_UNALIGNED_ID(StoreAddr, DefID, 8, __dfisan::AddrIsInSafeUnalignedRegion(StoreAddr));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_unaligned_store_id_16(uptr StoreAddr, u16 DefID) {
  SET_COND_UNALIGNED_ID(StoreAddr, DefID, 16, __dfisan::AddrIsInSafeUnalignedRegion(StoreAddr));
}

// conditional aligned or unaligned
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_aligned_or_unaligned_store_id_n(uptr StoreAddr, u64 Size, u16 DefID) {
  SET_COND_ALIGNED_OR_UNALIGNED_ID(StoreAddr, DefID, Size);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_aligned_or_unaligned_store_id_1(uptr StoreAddr, u16 DefID) {
  SET_COND_ALIGNED_OR_UNALIGNED_ID(StoreAddr, DefID, 1);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_aligned_or_unaligned_store_id_2(uptr StoreAddr, u16 DefID) {
  SET_COND_ALIGNED_OR_UNALIGNED_ID(StoreAddr, DefID, 2);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_aligned_or_unaligned_store_id_4(uptr StoreAddr, u16 DefID) {
  SET_COND_ALIGNED_OR_UNALIGNED_ID(StoreAddr, DefID, 4);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_aligned_or_unaligned_store_id_8(uptr StoreAddr, u16 DefID) {
  SET_COND_ALIGNED_OR_UNALIGNED_ID(StoreAddr, DefID, 8);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_aligned_or_unaligned_store_id_16(uptr StoreAddr, u16 DefID) {
  SET_COND_ALIGNED_OR_UNALIGNED_ID(StoreAddr, DefID, 16);
}

/* --- Check functions --- */
// aligned
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_check_ids_n(uptr LoadAddr, u64 Size, u32 Argc, ...) {
  CHECK_ALIGNED_ID_LIST(AlignAddr(LoadAddr), Argc, getShadowAlignedSize(Size));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_check_ids_1(uptr LoadAddr, u32 Argc, ...) {
  CHECK_ALIGNED_ID_LIST(AlignAddr(LoadAddr), Argc, 1);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_check_ids_2(uptr LoadAddr, u32 Argc, ...) {
  CHECK_ALIGNED_ID_LIST(AlignAddr(LoadAddr), Argc, 1);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_check_ids_4(uptr LoadAddr, u32 Argc, ...) {
  CHECK_ALIGNED_ID_LIST(AlignAddr(LoadAddr), Argc, 1);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_check_ids_8(uptr LoadAddr, u32 Argc, ...) {
  CHECK_ALIGNED_ID_LIST(AlignAddr(LoadAddr), Argc, 2);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_check_ids_16(uptr LoadAddr, u32 Argc, ...) {
  CHECK_ALIGNED_ID_LIST(AlignAddr(LoadAddr), Argc, 4);
}

// unaligned
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_unaligned_check_ids_n(uptr LoadAddr, u64 Size, u32 Argc, ...) {
  CHECK_UNALIGNED_ID_LIST(LoadAddr, Argc, Size);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_unaligned_check_ids_1(uptr LoadAddr, u32 Argc, ...) {
  CHECK_UNALIGNED_ID_LIST(LoadAddr, Argc, 1);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_unaligned_check_ids_2(uptr LoadAddr, u32 Argc, ...) {
  CHECK_UNALIGNED_ID_LIST(LoadAddr, Argc, 2);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_unaligned_check_ids_4(uptr LoadAddr, u32 Argc, ...) {
  CHECK_UNALIGNED_ID_LIST(LoadAddr, Argc, 4);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_unaligned_check_ids_8(uptr LoadAddr, u32 Argc, ...) {
  CHECK_UNALIGNED_ID_LIST(LoadAddr, Argc, 8);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_unaligned_check_ids_16(uptr LoadAddr, u32 Argc, ...) {
  CHECK_UNALIGNED_ID_LIST(LoadAddr, Argc, 16);
}

// aligned or unaligned
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_or_unaligned_check_ids_n(uptr LoadAddr, u64 Size, u32 Argc, ...) {
  CHECK_ALIGNED_OR_UNALIGNED_ID_LIST(LoadAddr, Argc, Size);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_or_unaligned_check_ids_1(uptr LoadAddr, u32 Argc, ...) {
  CHECK_ALIGNED_OR_UNALIGNED_ID_LIST(LoadAddr, Argc, 1);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_or_unaligned_check_ids_2(uptr LoadAddr, u32 Argc, ...) {
  CHECK_ALIGNED_OR_UNALIGNED_ID_LIST(LoadAddr, Argc, 2);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_or_unaligned_check_ids_4(uptr LoadAddr, u32 Argc, ...) {
  CHECK_ALIGNED_OR_UNALIGNED_ID_LIST(LoadAddr, Argc, 4);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_or_unaligned_check_ids_8(uptr LoadAddr, u32 Argc, ...) {
  CHECK_ALIGNED_OR_UNALIGNED_ID_LIST(LoadAddr, Argc, 8);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_aligned_or_unaligned_check_ids_16(uptr LoadAddr, u32 Argc, ...) {
  CHECK_ALIGNED_OR_UNALIGNED_ID_LIST(LoadAddr, Argc, 16);
}

// conditional aligned
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_aligned_check_ids_n(uptr LoadAddr, u64 Size, u32 Argc, ...) {
  CHECK_COND_ALIGNED_ID_LIST(AlignAddr(LoadAddr), Argc, getShadowAlignedSize(Size), __dfisan::AddrIsInSafeAlignedRegion(LoadAddr));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_aligned_check_ids_1(uptr LoadAddr, u32 Argc, ...) {
  CHECK_COND_ALIGNED_ID_LIST(AlignAddr(LoadAddr), Argc, 1, __dfisan::AddrIsInSafeAlignedRegion(LoadAddr));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_aligned_check_ids_2(uptr LoadAddr, u32 Argc, ...) {
  CHECK_COND_ALIGNED_ID_LIST(AlignAddr(LoadAddr), Argc, 1, __dfisan::AddrIsInSafeAlignedRegion(LoadAddr));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_aligned_check_ids_4(uptr LoadAddr, u32 Argc, ...) {
  CHECK_COND_ALIGNED_ID_LIST(AlignAddr(LoadAddr), Argc, 1, __dfisan::AddrIsInSafeAlignedRegion(LoadAddr));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_aligned_check_ids_8(uptr LoadAddr, u32 Argc, ...) {
  CHECK_COND_ALIGNED_ID_LIST(AlignAddr(LoadAddr), Argc, 2, __dfisan::AddrIsInSafeAlignedRegion(LoadAddr));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_aligned_check_ids_16(uptr LoadAddr, u32 Argc, ...) {
  CHECK_COND_ALIGNED_ID_LIST(AlignAddr(LoadAddr), Argc, 4, __dfisan::AddrIsInSafeAlignedRegion(LoadAddr));
}

// conditional unaligned
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_unaligned_check_ids_n(uptr LoadAddr, u64 Size, u32 Argc, ...) {
  CHECK_COND_UNALIGNED_ID_LIST(LoadAddr, Argc, Size, __dfisan::AddrIsInSafeUnalignedRegion(LoadAddr));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_unaligned_check_ids_1(uptr LoadAddr, u32 Argc, ...) {
  CHECK_COND_UNALIGNED_ID_LIST(LoadAddr, Argc, 1, __dfisan::AddrIsInSafeUnalignedRegion(LoadAddr));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_unaligned_check_ids_2(uptr LoadAddr, u32 Argc, ...) {
  CHECK_COND_UNALIGNED_ID_LIST(LoadAddr, Argc, 2, __dfisan::AddrIsInSafeUnalignedRegion(LoadAddr));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_unaligned_check_ids_4(uptr LoadAddr, u32 Argc, ...) {
  CHECK_COND_UNALIGNED_ID_LIST(LoadAddr, Argc, 4, __dfisan::AddrIsInSafeUnalignedRegion(LoadAddr));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_unaligned_check_ids_8(uptr LoadAddr, u32 Argc, ...) {
  CHECK_COND_UNALIGNED_ID_LIST(LoadAddr, Argc, 8, __dfisan::AddrIsInSafeUnalignedRegion(LoadAddr));
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_unaligned_check_ids_16(uptr LoadAddr, u32 Argc, ...) {
  CHECK_COND_UNALIGNED_ID_LIST(LoadAddr, Argc, 16, __dfisan::AddrIsInSafeUnalignedRegion(LoadAddr));
}

// conditional aligned or unaligned
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_aligned_or_unaligned_check_ids_n(uptr LoadAddr, u64 Size, u32 Argc, ...) {
  CHECK_COND_ALIGNED_OR_UNALIGNED_ID_LIST(LoadAddr, Argc, Size);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_aligned_or_unaligned_check_ids_1(uptr LoadAddr, u32 Argc, ...) {
  CHECK_COND_ALIGNED_OR_UNALIGNED_ID_LIST(LoadAddr, Argc, 1);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_aligned_or_unaligned_check_ids_2(uptr LoadAddr, u32 Argc, ...) {
  CHECK_COND_ALIGNED_OR_UNALIGNED_ID_LIST(LoadAddr, Argc, 2);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_aligned_or_unaligned_check_ids_4(uptr LoadAddr, u32 Argc, ...) {
  CHECK_COND_ALIGNED_OR_UNALIGNED_ID_LIST(LoadAddr, Argc, 4);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_aligned_or_unaligned_check_ids_8(uptr LoadAddr, u32 Argc, ...) {
  CHECK_COND_ALIGNED_OR_UNALIGNED_ID_LIST(LoadAddr, Argc, 8);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_cond_aligned_or_unaligned_check_ids_16(uptr LoadAddr, u32 Argc, ...) {
  CHECK_COND_ALIGNED_OR_UNALIGNED_ID_LIST(LoadAddr, Argc, 16);
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

} // namespace __dfisan

// ------------------ Interface ---------------------
using namespace __dfisan;

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_init() {
  Report("INFO: Initialize dfisan\n");
  DfisanInitInternal();
}
