#include "dfisan/dfisan_errors.h"
#include "dfisan/dfisan_mapping.h"
#include "sanitizer_common/sanitizer_common.h"
//#include "sanitizer_common/sanitizer_internal_defs.h"

#include <stdlib.h>

using namespace __sanitizer;

namespace __dfisan {

void ReportError(uptr Addr) {
  u16 *shadow_memory = (u16 *)MemToShadow(Addr);
  u16 ID = *shadow_memory;
  Decorator d;
  Printf("%s", d.Error());
  Report("ERROR: Invalid access at %p { ID(%d) at %p }\n", (void *)Addr, ID, (void *)shadow_memory);
  Printf("%s", d.Default());
}

void ReportInvalidUseError(uptr LoadAddr, u16 Argc, va_list IDList) {
  u16 *shadow_memory = (u16 *)MemToShadow(LoadAddr);
  u16 InvalidID = *shadow_memory;
  Decorator d;
  Printf("%s", d.Error());
  Report("ERROR: Invalid access at %p { ID(%d) at %p }\n", (void *)LoadAddr, InvalidID, (void *)shadow_memory);
  if (Argc != 0) {
    Report("Expected DefIDs: { ");
    for (u16 i = 0; i < Argc - 1; i++) {
      u16 ExpectedID = va_arg(IDList, u16);
      Printf("%d, ", ExpectedID);
    }
    Printf("%d", va_arg(IDList, u16));
    Printf(" }\n");
  }
  Printf("%s", d.Default());
  exit(1);
}

} // namespace __dfisan