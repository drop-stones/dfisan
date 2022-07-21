#ifndef LLVM_ANALYSIS_DG_PASSES_USEDEFLOGGER_H
#define LLVM_ANALYSIS_DG_PASSES_USEDEFLOGGER_H

#include "sqlite3.h"
#include "llvm/Support/raw_ostream.h"
#include "dg/Passes/UseDefBuilder.h"

namespace dg {
namespace debug {

class UseDefLogger {
public:
  UseDefLogger(llvm::Module &M);
  ~UseDefLogger();

  void logDefInfo(UseDefBuilder *Builder);

private:
  const std::string DBTableName = "DefInfoTable";
  const std::string DBFilename;
  sqlite3 *DB;

  std::string getModuleName(llvm::Module &M);
  inline bool isEnabled();
  void logSingleDefInfo(DefID ID, llvm::StringRef &Filename, unsigned Line = 0, unsigned Column = 0); 
};

} // namespace debug
} // namespace dg

#endif