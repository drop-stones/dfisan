
#include "UseDefAnalysis/UseDefUtils.h"

namespace {
std::string removeSuffix(StringRef FileName) {
  std::string Ret;
  if (FileName.endswith(".ll")) {
    Ret = FileName.drop_back(3).str();
  } else if (FileName.endswith(".c")) {
    Ret = FileName.drop_back(2).str();
  } else {
    Ret = FileName.str();
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