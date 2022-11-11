#ifndef LLVM_ANALYSIS_SVF_DFISAN_DEFUSELOGGER_H_
#define LLVM_ANALYSIS_SVF_DFISAN_DEFUSELOGGER_H_

#include "sqlite3.h"
#include "llvm/Support/raw_ostream.h"
#include "Dfisan/ProtectInfo.h"

namespace SVF {

class DefUseLogger {
public:
  DefUseLogger(llvm::Module &M);
  ~DefUseLogger();

  void logProtectInfo(ProtectInfo *ProtectInfo);

private:
  const std::string DBTableName = "DefInfoTable";
  const std::string DBFilename;
  sqlite3 *DB;

  std::string getModuleName(llvm::Module &M);
  inline bool isEnabled();
  void logSingleDefInfo(DefID ID, llvm::StringRef &Filename, unsigned Line = 0, unsigned Column = 0);
};

} // namespace SVF

#endif
