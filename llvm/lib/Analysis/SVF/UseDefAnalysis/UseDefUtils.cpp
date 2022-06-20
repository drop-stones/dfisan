
#include "UseDefAnalysis/UseDefUtils.h"

namespace {
std::string removeSuffix(StringRef Filename) {
  std::string Ret;
  if (Filename.endswith(".ll")) {
    Ret = Filename.drop_back(3).str();
  } else if (Filename.endswith(".bc")) {
    Ret = Filename.drop_back(3).str();
  } else if (Filename.endswith(".c")) {
    Ret = Filename.drop_back(2).str();
  } else {
    Ret = Filename.str();
  }
  return Ret;
}
} // anonymous namespace

std::string getModuleName(Module &M) {
  StringRef ModuleIdentifier = M.getModuleIdentifier();
  return removeSuffix(ModuleIdentifier);
}

std::string getModuleName(SVFModule *M) {
  StringRef ModuleIdentifier = M->getModuleIdentifier();
  return removeSuffix(ModuleIdentifier);
}


bool isMemcpy(const StmtSVFGNode *Node) {
  if (Node->getInst() == nullptr)
    return false;
  return llvm::isa<const llvm::MemCpyInst>(Node->getInst());
}
bool isMemset(const StmtSVFGNode *Node) {
  if (Node->getInst() == nullptr)
    return false;
  return llvm::isa<const llvm::MemSetInst>(Node->getInst());
}
bool isMemcpyOrMemset(const StmtSVFGNode *Node) {
  return isMemcpy(Node) || isMemset(Node);
}
