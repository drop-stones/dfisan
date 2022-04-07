#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_internal_defs.h"

#include <stdlib.h>
#include <stdarg.h>

using namespace __sanitizer;

#define SIZE 8192
static u32 RDT[SIZE];
inline void setRDT(u32 *Addr, u32 ID) {
  RDT[(u64)Addr % SIZE] = ID;
}
inline bool checkRDT(u32 *Addr, u32 ID) {
  return RDT[(u64)Addr % SIZE] == ID;
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_store_id(u32 *StoreAddr, u32 DefID) {
  Report("INFO: Set DefID(%d) at %p\n", DefID, (void *)StoreAddr);
  setRDT(StoreAddr, DefID);
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_check_ids(u32 *LoadAddr, u32 Argc, ...) {
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