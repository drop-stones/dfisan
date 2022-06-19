
#ifndef LLVM_ANALYSIS_SVF_USEDEFANALYSIS_USEDEFLOGGER_H
#define LLVM_ANALYSIS_SVF_USEDEFANALYSIS_USEDEFLOGGER_H

#include "sqlite3.h"
#include <string>
#include "llvm/Support/raw_ostream.h"

namespace SVF {
class UseDefChain;

class UseDefLogger {
public:
  UseDefLogger(std::string ModuleName);
  ~UseDefLogger();
/*
   : DBFileName(ModuleName + ".sqlite3") {
    if (sqlite3_open(DBFileName.c_str(), &DB) != SQLITE_OK) {
      llvm::errs() << "Error: sqlite_open: Cannot open " << DBFileName << "\n";
      exit(1);
    }
  }

  ~UseDefLogger() {
    if (sqlite3_close(DB) != SQLITE_OK) {
      llvm::errs() << "Error: sqlite_close: Cannot close " << DBFileName << "\n";
      exit(1);
    }
  }
*/

  void logDefInfo(UseDefChain &UseDef);

private:
  const std::string DBTableName = "DefInfoTable";
  const std::string DBFileName;
  sqlite3 *DB;
};

} // namespace SVF

#endif