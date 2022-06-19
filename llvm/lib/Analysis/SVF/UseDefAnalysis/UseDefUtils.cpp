
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