//===-- dfisan_interceptors.h -----------------------------------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//
///
/// DFISan-private header for asan_interceptors.cpp
///
//===----------------------------------------------------------------------===//
#ifndef DFISAN_INTERCEPTORS_H
#define DFISAN_INTERCEPTORS_H

//#define ENSURE_DFISAN_INITED()                        \
  do {                                                \
    CHECK(__dfisan::dfisan_init_is_running == false); \
    if (UNLIKELY(__dfisan::dfisan_inited == false)) { \
    }                                                 \
  } while (0)

//#define COMMON_INTERCEPTOR_ENTER(func, ...) \
  if (__dfisan::dfisan_init_is_running)     \
    return REAL(func)(__VA_ARGS__);         \
  ENSURE_DFISAN_INITED();

namespace __dfisan {

void InitializeDfisanInterceptors();

} // namespace __dfisan

#endif