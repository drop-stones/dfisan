#include "dg/Passes/UseDefLogger.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#define DEBUG_TYPE "usedef-log"

using namespace llvm;
using namespace dg;

static cl::opt<std::string> OutputFilename(
  "output-filename",
  cl::init("-"),
  cl::desc("<output filename>"),
  cl::Optional
);

namespace dg {
namespace debug {

std::string
UseDefLogger::getModuleName(llvm::Module &M) {
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

bool
UseDefLogger::isEnabled() {
  return DebugFlag == true && isCurrentDebugType(DEBUG_TYPE);
}

UseDefLogger::UseDefLogger(llvm::Module &M)
  : DBFilename( (OutputFilename == "-" ? getModuleName(M) : OutputFilename) + ".sqlite3") {
  if (!isEnabled())
    return;
  LLVM_DEBUG(llvm::errs() << "DBFilename: " << DBFilename << "\n");
  if (sqlite3_open(DBFilename.c_str(), &DB) != SQLITE_OK) {
    llvm::errs() << "Error: sqlite_open: Cannot open " << DBFilename << "\n";
    exit(1);
  }
}

UseDefLogger::~UseDefLogger() {
  if (!isEnabled())
    return;
  if (sqlite3_close(DB) != SQLITE_OK) {
    llvm::errs() << "Error: sqlite_close: Cannot close " << DBFilename << "\n";
    exit(1);
  }
}

void
UseDefLogger::logDefInfo(UseDefBuilder *Builder) {
  if (!isEnabled())
    return;
  
  char *ErrMsg;
  sqlite3_exec(DB, ("DROP TABLE IF EXISTS " + DBTableName).c_str(), nullptr, nullptr, &ErrMsg);
  sqlite3_exec(DB, ("CREATE TABLE " + DBTableName + " (DefID INTEGER , line INTEGER, column INTEGER, filename TEXT, UNIQUE(DefID, line, column, filename))").c_str(), nullptr, nullptr, &ErrMsg);

  // Log global variable initializations.
  for (auto GI = Builder->glob_begin(); GI != Builder->glob_end(); GI++) {
    auto *GlobVal = (llvm::Value *)Builder->getDDA()->getValue(*GI);
    DefID ID = Builder->getDefID(GlobVal);
    std::string Valname = GlobVal->getNameOrAsOperand();
    StringRef Filename = Valname;
    logSingleDefInfo(ID, Filename);
  }

  // Log store instructions.
  for (auto DI = Builder->def_begin(); DI != Builder->def_end(); DI++) {
    auto *Def = (*DI)->getValue();
    DefID ID = Builder->getDefID(Def);

    unsigned Line = 0, Column = 0;
    StringRef Filename;
    if (auto *Inst = llvm::dyn_cast<llvm::Instruction>(Def)) {
      auto &DebugLoc = Inst->getDebugLoc();
      if (DebugLoc) {
        Line = DebugLoc->getLine();
        Column = DebugLoc->getColumn();
        Filename = DebugLoc->getScope()->getFilename();
      }
    } else {
      assert(false && "No support value");
    }
    logSingleDefInfo(ID, Filename, Line, Column);
  }
}

void
UseDefLogger::logSingleDefInfo(DefID ID, llvm::StringRef &Filename, unsigned Line, unsigned Column) {
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

} // namespace debug
} // namespace dg