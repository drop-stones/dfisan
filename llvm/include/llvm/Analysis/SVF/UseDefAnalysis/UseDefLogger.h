
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

  void logDefInfo(UseDefChain &UseDef);

private:
  const std::string DBTableName = "DefInfoTable";
  const std::string DBFileName;
  sqlite3 *DB;
};

} // namespace SVF

#endif