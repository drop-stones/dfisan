#include "dfisan/dfisan_errors.h"
#include "dfisan/dfisan_mapping.h"
#include "sanitizer_common/sanitizer_common.h"
//#include "sanitizer_common/sanitizer_internal_defs.h"

using namespace __sanitizer;

namespace __dfisan {

void ReportError(uptr Addr) {
  u16 *shadow_memory = (u16 *)MemToShadow(Addr);
  u16 ID = *shadow_memory;
  //Decorator d;
  //Printf("%s", d.Error());
  Report("ERROR: Invalid access at %p { ID(%d) at %p }\n", (void *)Addr, ID, (void *)shadow_memory);
}

} // namespace __dfisan