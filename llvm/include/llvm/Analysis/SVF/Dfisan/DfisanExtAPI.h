#ifndef DFISAN_EXTAPI_H_
#define DFISAN_EXTAPI_H_

#include "Util/BasicTypes.h"
#include <string>
#include <unordered_map>

namespace SVF {

class DfisanExtAPI {
public:
  enum class ExtFunType {
    EXT_DEF,
    EXT_USE,
    EXT_CALLOC,
    EXT_OTHER,
  };

  enum class AccessPosition {
    RET,
    ARG,
    VARARG,
    NONE,
  };

  struct ExtFun {
    std::string FnName;
    ExtFunType Ty;
    AccessPosition Pos;
    unsigned ArgPos;
    unsigned SizePos;
    bool IsStrCpy;
    static constexpr unsigned DefaultPos = UINT32_MAX;

    ExtFun(std::string FnName, ExtFunType Ty, AccessPosition Pos, unsigned ArgPos, unsigned SizePos, bool IsStrCpy)
      : FnName(FnName), Ty(Ty), Pos(Pos), ArgPos(ArgPos), SizePos(SizePos), IsStrCpy(IsStrCpy) {}
    ExtFun() : ExtFun("", ExtFunType::EXT_OTHER, AccessPosition::NONE, DefaultPos, DefaultPos, DefaultPos) {}

    bool hasSizePos() const { return SizePos != DefaultPos; }
    bool isStrCpy() const { return IsStrCpy; }
  };

  std::unordered_map<std::string, ExtFun> ExtFunMap;

private:
  static DfisanExtAPI *dfisanExtAPI;

  /// Constructor
  DfisanExtAPI() {
    initExtFunMap();
  };

  void initExtFunMap() {
    ///
    // calloc
    ///
    addExtFun("calloc",                         ExtFunType::EXT_CALLOC, AccessPosition::RET);
    addExtFun("__dfisan_safe_aligned_calloc",   ExtFunType::EXT_CALLOC, AccessPosition::RET);
    addExtFun("__dfisan_safe_unaligned_calloc", ExtFunType::EXT_CALLOC, AccessPosition::RET);

    ///
    // Memory block functions
    ///
    addExtFun("llvm.memcpy.p0i8.p0i8.i64", ExtFunType::EXT_DEF, AccessPosition::ARG, 0, 2);
    addExtFun("llvm.memcpy.p0i8.p0i8.i32", ExtFunType::EXT_DEF, AccessPosition::ARG, 0, 2);
    addExtFun("llvm.memset.p0i8.i64",      ExtFunType::EXT_DEF, AccessPosition::ARG, 0, 2);

    ///
    // String handling functions
    ///
    addExtFun("read",   ExtFunType::EXT_DEF, AccessPosition::ARG, 1, 2);
    addExtFun("fgets",  ExtFunType::EXT_DEF, AccessPosition::ARG, 0, 1);
    addExtFun("__isoc99_sscanf", ExtFunType::EXT_DEF, AccessPosition::VARARG, 2);
    addExtFun("strcpy", ExtFunType::EXT_DEF, AccessPosition::ARG, 0, 1, true);
  }

  void addExtFun(std::string FnName, ExtFunType Type, AccessPosition Pos, unsigned ArgPos = ExtFun::DefaultPos, unsigned SizeArg = ExtFun::DefaultPos, bool IsStrCpy = false) {
    ExtFunMap[FnName] = {FnName, Type, Pos, ArgPos, SizeArg, IsStrCpy};
  }

public:
  /// Singleton constructor
  static DfisanExtAPI *getDfisanExtAPI();
  static void destroy();

  /// Check ExtFun from SVFFunction
  bool isExtFun(const SVFFunction *F);
  bool isExtDefFun(const SVFFunction *F);
  bool isExtUseFun(const SVFFunction *F);

  /// Check ExtFun from function name
  bool isExtFun(std::string FnName);
  bool isExtDefFun(std::string FnName);
  bool isExtUseFun(std::string FnName);

  const ExtFun &getExtFun(const SVFFunction *F);
  const ExtFun &getExtFun(std::string FnName);

  /// Check the inst is calloc call
  bool isCallocCall(const Instruction *Inst);
};

} // namespace SVF

#endif
