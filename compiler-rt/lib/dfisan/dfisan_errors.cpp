#include "dfisan/dfisan_errors.h"
#include "dfisan/dfisan_mapping.h"
#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_stacktrace.h"
//#include "sanitizer_common/sanitizer_internal_defs.h"

#include "sqlite3.h"
#include <stdlib.h>
#include <assert.h>
#include <limits.h>   // PATH_MAX
#include <unistd.h>   // readlink
#include <string.h>   // strcat

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

void PrintDefInfo(u16 Argc, va_list IDList) {
  if (Argc == 0)
    return;

  char Suffix[] = ".sqlite3";
  char FilePath[PATH_MAX];
  ssize_t len = readlink("/proc/self/exe", FilePath, sizeof(FilePath));
  FilePath[len] = '\0';
  char *Sqlite3FilePath = strcat(FilePath, Suffix);

  if (access(Sqlite3FilePath, F_OK) == 0) {   // Print DefInfo in sqlite3
    // Printf("Exists: %s\n", Sqlite3FilePath);
    sqlite3 *DB;
    if (sqlite3_open(Sqlite3FilePath, &DB) != SQLITE_OK) {
      Printf("Error: sqlite_open: Cannot open %s\n", Sqlite3FilePath);
      exit(1);
    }
    Printf("%s | %s\n",
      "   DefID" /* width = 8 */,
      "Location or Data");
    for (u16 i = 0; i < Argc; i++) {
      u16 ExpectedID = va_arg(IDList, u16);
      sqlite3_stmt *SqliteStmt;
      char SelectStmt[] = "SELECT DefID, line, column, filename FROM DefInfoTable WHERE DefID = @defid";
      sqlite3_prepare(DB, SelectStmt, sizeof(SelectStmt), &SqliteStmt, nullptr);

      int Idx = sqlite3_bind_parameter_index(SqliteStmt, "@defid");
      sqlite3_bind_int(SqliteStmt, Idx, ExpectedID);

      while (1) {
        int Step = sqlite3_step(SqliteStmt);
        if (Step == SQLITE_ROW) {
          if (sqlite3_column_int(SqliteStmt, 1) == 0 && sqlite3_column_int(SqliteStmt, 2) == 0)
            Printf("%8d | %s\n", sqlite3_column_int(SqliteStmt, 0), sqlite3_column_text(SqliteStmt, 3));
          else
            Printf("%8d | %s:%d:%d\n",
              sqlite3_column_int(SqliteStmt, 0), sqlite3_column_text(SqliteStmt, 3),
              sqlite3_column_int(SqliteStmt, 1), sqlite3_column_int(SqliteStmt, 2));
        } else {
          break;
        }
      }

      sqlite3_reset(SqliteStmt);
      sqlite3_finalize(SqliteStmt);
    }
    sqlite3_close(DB);
  } else {      // Print all DefIDs
    Report("{");
    for (u16 i = 0; i < Argc - 1; i++) {
      u16 ExpectedID = va_arg(IDList, u16);
      Printf("%d, ", ExpectedID);
    }
    Printf("%d", va_arg(IDList, u16));
    Printf(" }\n");
  }
}

void ReportInvalidUseError(uptr LoadAddr, u16 Argc, va_list IDList, uptr pc, uptr bp) {
  u16 *shadow_memory = (u16 *)MemToShadow(LoadAddr);
  u16 InvalidID = *shadow_memory;
  Decorator d;
  Printf("\n%s", d.Error());
  Printf("ERROR: Invalid access at %p { ID(%d) at %p }\n", (void *)LoadAddr, InvalidID, (void *)shadow_memory);
  Printf("%s", d.Default());

  Printf("\n%s", d.DefInfo());
  Printf("Expected DefIDs:\n");
  Printf("%s", d.Default());
  PrintDefInfo(Argc, IDList);

  Printf("\n%s", d.StackTrace());
  Printf("StackTrace:\n");
  Printf("%s", d.Default());
  BufferedStackTrace stack;
  stack.Unwind(pc, bp, nullptr, common_flags()->fast_unwind_on_fatal);
  stack.Print();

  exit(1);
}

} // namespace __dfisan