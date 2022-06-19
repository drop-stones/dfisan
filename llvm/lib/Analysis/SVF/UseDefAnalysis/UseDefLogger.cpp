
#include "UseDefAnalysis/UseDefLogger.h"
#include "UseDefAnalysis/UseDefChain.h"

#include "llvm/Support/Debug.h"
#define DEBUG_TYPE "usedef-log"

using namespace llvm;
using namespace SVF;

UseDefLogger::UseDefLogger(std::string ModuleName) : DBFileName(ModuleName + ".sqlite3") {
  if (DebugFlag == false || !isCurrentDebugType(DEBUG_TYPE))
    return;
  if (sqlite3_open(DBFileName.c_str(), &DB) != SQLITE_OK) {
    llvm::errs() << "Error: sqlite_open: Cannot open " << DBFileName << "\n";
    exit(1);
  }
}

UseDefLogger::~UseDefLogger() {
  if (DebugFlag == false || !isCurrentDebugType(DEBUG_TYPE))
    return;
  if (sqlite3_close(DB) != SQLITE_OK) {
    llvm::errs() << "Error: sqlite_close: Cannot close " << DBFileName << "\n";
    exit(1);
  }
}

void UseDefLogger::logDefInfo(UseDefChain &UseDef) {
  if (DebugFlag == false || !isCurrentDebugType(DEBUG_TYPE))
    return;

  char *ErrMsg;
  sqlite3_exec(DB, ("DROP TABLE IF EXISTS " + DBTableName).c_str(), nullptr, nullptr, &ErrMsg);
  sqlite3_exec(DB, ("CREATE TABLE " + DBTableName + " (DefID INTEGER , line INTEGER, column INTEGER, filename TEXT, UNIQUE(DefID, line, column, filename))").c_str(), nullptr, nullptr, &ErrMsg);
  for (const auto &Iter : UseDef.getDefInfoMap()) {
    const auto *StoreNode = Iter.first;
    const auto DefInfo = Iter.second;
    NodeID DefID = DefInfo.ID;

    const auto *Inst = StoreNode->getInst();
    unsigned Line = 0, Column = 0;
    StringRef Filename;
    std::string Valname;
    if (Inst != nullptr) {
      const auto &DebugLoc = Inst->getDebugLoc();
      if (DebugLoc) {
        Line = DebugLoc->getLine();
        Column = DebugLoc->getColumn();
        Filename = DebugLoc->getScope()->getFilename();
        // LLVM_DEBUG(errs() << "Line: " << Line << ", Column: " << Column << ", Filename: " << Filename << "\n");
      } else {
        // LLVM_DEBUG(errs() << "DebugLoc does not exist: " << *Inst << "\n");
      }
    } else {
      // LLVM_DEBUG(errs() << "Inst == nullptr: " << StoreNode->toString() << "\n");
      const auto *Val = StoreNode->getValue();
      if (Val != nullptr) {
        Valname = Val->getNameOrAsOperand();
        Filename = Valname;
      }
    }

    // Prepare sqlite3 statement.
    std::string StmtString;
    raw_string_ostream StmtStream{StmtString};
    StmtStream << "INSERT OR IGNORE INTO " << DBTableName << " VALUES (" << DefID << ", " << Line << ", " << Column << ", \'" << Filename << "\')";
    if (sqlite3_exec(DB, StmtString.c_str(), nullptr, nullptr, &ErrMsg) != SQLITE_OK) {
      errs() << "Error: sqlite3_exec: " << ErrMsg << "\n";
      errs() << "StmtString: " << StmtString << "\n";
      exit(1);
    }
  /*
    sqlite3_stmt *SqliteStmt;
    StmtStream << "INSERT OR IGNORE INTO " << DBTableName << " VALUES (?, ?, ?, ?)";
    sqlite3_prepare(DB, StmtString.c_str(), StmtString.length(), &SqliteStmt, nullptr);
    sqlite3_bind_int(SqliteStmt, 1, DefID);
    sqlite3_bind_int(SqliteStmt, 2, Line);
    sqlite3_bind_int(SqliteStmt, 3, Column);
    sqlite3_bind_text(SqliteStmt, 4, Filename.str().c_str(), Filename.size(), SQLITE_STATIC);
    sqlite3_step(SqliteStmt);
    sqlite3_reset(SqliteStmt);
    sqlite3_finalize(SqliteStmt);
  */

    // LLVM_DEBUG(errs() << "StmtString: " << StmtString << "\n");
  }
}