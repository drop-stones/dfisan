#include "Dfisan/DefUseLogger.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#define DEBUG_TYPE "def-use-log"

using namespace llvm;
using namespace SVF;

static cl::opt<std::string> OutputFilename(
  "output-filename",
  cl::init("-"),
  cl::desc("<output filename>"),
  cl::Optional
);

std::string
DefUseLogger::getModuleName(llvm::Module &M) {
  StringRef ModuleID = M.getModuleIdentifier();
  std::string ModuleName;
  if (ModuleID.endswith(".ll")) {
    ModuleName = ModuleID.drop_back(3).str();
  } else if (ModuleID.endswith(".bc")) {
    ModuleName = ModuleID.drop_back(3).str();
  } else if (ModuleID.endswith(".c")) {
    ModuleName = ModuleID.drop_back(2).str();
  } else {
    ModuleName = ModuleID.str();
  }
  return ModuleName;
}

bool DefUseLogger::isEnabled() {
  return DebugFlag == true && isCurrentDebugType(DEBUG_TYPE);
}

DefUseLogger::DefUseLogger(llvm::Module &M)
  : DBFilename( (OutputFilename == "-" ? getModuleName(M) : OutputFilename) + ".sqlite3" )
{
  if (!isEnabled())
    return;
  LLVM_DEBUG(llvm::dbgs() << "DBFilename: " << DBFilename << "\n");
  if (sqlite3_open(DBFilename.c_str(), &DB) != SQLITE_OK) {
    llvm::errs() << "Error: sqlite_open: Cannot open " << DBFilename << "\n";
    exit(1);
  }
}

DefUseLogger::~DefUseLogger() {
  if (!isEnabled())
    return;
  if (sqlite3_close(DB) != SQLITE_OK) {
    llvm::errs() << "Error: sqlite_close: Cannot close " << DBFilename << "\n";
    exit(1);
  }
}

void
DefUseLogger::logProtectInfo(ProtectInfo *ProtectInfo) {
  if (!isEnabled())
    return;
  
  char *ErrMsg;
  sqlite3_exec(DB, ("DROP TABLE IF EXISTS " + DBTableName).c_str(), nullptr, nullptr, &ErrMsg);
  sqlite3_exec(DB, ("CREATE TABLE " + DBTableName + " (DefID INTEGER , line INTEGER, column INTEGER, filename TEXT, UNIQUE(DefID, line, column, filename))").c_str(), nullptr, nullptr, &ErrMsg);

  // Log Def instructions.
  for (auto &Iter : ProtectInfo->getDefToInfo()) {
    llvm::Value *Def = Iter.first;
    DefID ID = ProtectInfo->getDefID(Def);
    if (auto *GlobVar = dyn_cast<GlobalVariable>(Def)) {
      StringRef Valname = GlobVar->getName();
      logSingleDefInfo(ID, Valname);
    } else if (auto *Inst = dyn_cast<Instruction>(Def)) {
      unsigned Line = 0, Column = 0;
      StringRef Filename;
      if (auto &DebugLoc = Inst->getDebugLoc()) {
        Line = DebugLoc->getLine();
        Column = DebugLoc->getColumn();
        Filename = DebugLoc->getScope()->getFilename();
      }
      logSingleDefInfo(ID, Filename, Line, Column);
    } else {
      llvm_unreachable("No support def value");
    }
  }
}

void
DefUseLogger::logSingleDefInfo(DefID ID, llvm::StringRef &Filename, unsigned Line, unsigned Column) {
  // Prepare sqlite3 statement
  std::string StmtString;
  raw_string_ostream StmtStream{StmtString};

  sqlite3_stmt *SqliteStmt;
  StmtStream << "INSERT OR IGNORE INTO " << DBTableName << " VALUES (?, ?, ?, ?)";
  sqlite3_prepare(DB, StmtString.c_str(), StmtString.length(), &SqliteStmt, nullptr);
  sqlite3_bind_int(SqliteStmt, 1, ID);
  sqlite3_bind_int(SqliteStmt, 2, Line);
  sqlite3_bind_int(SqliteStmt, 3, Column);
  sqlite3_bind_text64(SqliteStmt, 4, Filename.str().c_str(), Filename.size() + 1, SQLITE_TRANSIENT, SQLITE_UTF8);
  sqlite3_step(SqliteStmt);
  sqlite3_reset(SqliteStmt);
  sqlite3_finalize(SqliteStmt);
}
