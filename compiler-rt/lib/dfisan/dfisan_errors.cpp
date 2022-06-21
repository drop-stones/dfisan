#include "dfisan/dfisan_errors.h"
#include "dfisan/dfisan_mapping.h"
#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_stacktrace.h"
//#include "sanitizer_common/sanitizer_internal_defs.h"

#include <stdlib.h>
#include <assert.h>

using namespace __sanitizer;

void BufferedStackTrace::UnwindImpl(
  uptr pc, uptr bp, void *context, bool request_fast, u32 max_depth) {
  // TODO: get stack_bottom and stack_top
  uptr stack_bottom, stack_top;
  GetThreadStackTopAndBottom(false, &stack_top, &stack_bottom);
  Report("Stack bottom: %p, Stack top: %p, IsValidFrame=%d\n", (void *)stack_bottom, (void *)stack_top, IsValidFrame(bp, stack_top, stack_bottom));
  if (stack_top != stack_bottom) {
    int local;
    assert(stack_bottom <= (uptr)&local && (uptr)&local < stack_top);
  }
  Unwind(max_depth, pc, bp, nullptr, stack_top, stack_bottom, true);
}

namespace __dfisan {

void ReportInvalidUseError(uptr LoadAddr, u16 Argc, va_list IDList, uptr pc, uptr bp) {
  u16 *shadow_memory = (u16 *)MemToShadow(LoadAddr);
  u16 InvalidID = *shadow_memory;
  Decorator d;
  Printf("%s", d.Error());
  Report("ERROR: Invalid access at %p { ID(%d) at %p }\n", (void *)LoadAddr, InvalidID, (void *)shadow_memory);
  if (Argc != 0) {
    // TODO: Get debug info from sqlite3 and print them.
    Report("Expected DefIDs: { ");
    for (u16 i = 0; i < Argc - 1; i++) {
      u16 ExpectedID = va_arg(IDList, u16);
      Printf("%d, ", ExpectedID);
    }
    Printf("%d", va_arg(IDList, u16));
    Printf(" }\n");
  }
  Printf("%s", d.Default());

  //GET_CALLER_PC_BP;
  BufferedStackTrace stack;
  stack.Unwind(pc, bp, nullptr, common_flags()->fast_unwind_on_fatal);
  stack.Print();

  exit(1);
}

} // namespace __dfisan