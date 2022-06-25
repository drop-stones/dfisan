include_directories(..)

# Runtime library sources and build flags.
set(DFISAN_RTL_SOURCES
  dfisan_rtl.cpp
  dfisan_interceptors.cpp
  dfisan_shadow_setup.cpp
  dfisan_errors.cpp
)

set(DFISAN_RTL_HEADERS
  dfisan_interceptors.h
  dfisan_interface_internal.h
  dfisan_internal.h
  dfisan_mapping.h
  dfisan_errors.h
)

set(DFISAN_COMMON_CFLAGS ${SANITIZER_COMMON_CFLAGS})
append_rtti_flag(OFF DFISAN_COMMON_CFLAGS)
append_list_if(COMPILER_RT_HAS_FFREESTANDIG_FLAG -ffreestanding DFISAN_COMMON_CFLAGS)

# Static runtime library
add_compiler_rt_component(dfisan)

# LLVMSqlite3 settings
get_target_property(LLVMSqlite3_INCLUDES LLVMSqlite3 INCLUDE_DIRECTORIES)

foreach(arch ${DFISAN_SUPPORTED_ARCH})
  set(DFISAN_CFLAGS ${DFISAN_COMMON_CFLAGS})
  append_list_if(COMPILER_RT_HAS_FPIE_FLAG -fPIE DFISAN_CFLAGS)
  add_compiler_rt_runtime(clang_rt.dfisan
    STATIC
    ARCHS ${arch}
    SOURCES ${DFISAN_RTL_SOURCES}
            $<TARGET_OBJECTS:RTInterception.${arch}>
            $<TARGET_OBJECTS:RTSanitizerCommon.${arch}>
            $<TARGET_OBJECTS:RTSanitizerCommonLibc.${arch}>
            $<TARGET_OBJECTS:RTSanitizerCommonSymbolizer.${arch}>
            $<TARGET_OBJECTS:LLVMSqlite3>
    ADDITIONAL_HEADERS ${DFISAN_RTL_HEADERS}
    CFLAGS ${DFISAN_CFLAGS}
    PARENT_TARGET dfisan
  )
  target_include_directories(clang_rt.dfisan-${arch} PRIVATE ${LLVMSqlite3_INCLUDES})

  # Export symbols
  add_sanitizer_rt_symbols(clang_rt.dfisan
    ARCHS ${arch}
    EXTRA dfisan.syms.extra
  )
  add_dependencies(dfisan
    clang_rt.dfisan-${arch}-symbols
  )
endforeach()