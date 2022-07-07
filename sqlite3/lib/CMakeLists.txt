add_llvm_library(LLVMSqlite3
  sqlite3.c
  LINK_LIBS dl pthread
)
