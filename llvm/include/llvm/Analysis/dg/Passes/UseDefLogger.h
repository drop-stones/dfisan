#ifndef LLVM_ANALYSIS_DG_PASSES_USEDEFLOGGER_H
#define LLVM_ANALYSIS_DG_PASSES_USEDEFLOGGER_H

#include "sqlite3.h"
#include "llvm/Support/raw_ostream.h"
#include "dg/Passes/UseDefBuilder.h"

namespace dg {
namespace debug {

class UseDefLogger {
public:
  UseDefLogger(std::string ModuleName);
  ~UseDefLogger();

  void logDefInfo(UseDefBuilder *Builder);

private:
  const std::string DBTableName = "DefInfoTable";
  const std::string DBFilename;
  sqlite3 *DB;

  inline bool isEnabled();
};

} // namespace debug
} // namespace dg

#endif