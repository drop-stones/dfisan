#include "Dfisan/DfisanExtAPI.h"

using namespace SVF;

DfisanExtAPI *DfisanExtAPI::dfisanExtAPI = nullptr;

DfisanExtAPI *DfisanExtAPI::getDfisanExtAPI() {
  if (dfisanExtAPI == nullptr)
    dfisanExtAPI = new DfisanExtAPI;
  return dfisanExtAPI;
}

void DfisanExtAPI::destroy() {
  if (dfisanExtAPI != nullptr) {
    delete dfisanExtAPI;
    dfisanExtAPI = nullptr;
  }
}

bool DfisanExtAPI::isExtFun(const SVFFunction *F) {
  std::string FnName = F->getName();
  return ExtFunMap.count(FnName) != 0;
}

bool DfisanExtAPI::isExtDefFun(const SVFFunction *F) {
  std::string FnName = F->getName();
  if (ExtFunMap.count(FnName))
    return ExtFunMap[FnName].Ty == ExtFunType::EXT_DEF;
  return false;
}

bool DfisanExtAPI::isExtUseFun(const SVFFunction *F) {
  std::string FnName = F->getName();
  if (ExtFunMap.count(FnName))
    return ExtFunMap[FnName].Ty == ExtFunType::EXT_USE;
  return false;
}

const DfisanExtAPI::ExtFun &DfisanExtAPI::getExtFun(const SVFFunction *F) {
  assert(isExtFun(F));
  std::string FnName = F->getName();
  return ExtFunMap[FnName];
}
