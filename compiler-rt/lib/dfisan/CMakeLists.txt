include_directories(..)

# Runtime library sources and build flags.
set(DFISAN_RTL_SOURCES
  dfisan.cpp
)

set(DFISAN_RTL_HEADERS
  # Put additional headers here if any
)

set(DFISAN_COMMON_CFLAGS ${SANITIZER_COMMON_CFLAGS})
append_rtti_flag(OFF DFISAN_COMMON_CFLAGS)
append_list_if(COMPILER_RT_HAS_FFREESTANDIG_FLAG -ffreestanding DFISAN_COMMON_CFLAGS)

# Static runtime library
add_compiler_rt_component(dfisan)

foreach(arch ${DFISAN_SUPPORTED_ARCH})
  set(DFISAN_CFLAGS ${DFISAN_COMMON_CFLAGS})
  append_list_if(COMPILER_RT_HAS_FPIE_FLAG -fPIE DFISAN_CFLAGS)
  add_compiler_rt_runtime(clang_rt.dfisan
    STATIC
    ARCHS ${arch}
    SOURCES ${DFISAN_RTL_SOURCES}
            $<TARGET_OBJECTS:RTSanitizerCommon.${arch}>
            $<TARGET_OBJECTS:RTSanitizerCommonLibc.${arch}>
    ADDITIONAL_HEADERS ${DFISAN_RTL_HEADERS}
    CFLAGS ${DFISAN_CFLAGS}
    PARENT_TARGET dfisan
  )

  # Export symbols
  add_sanitizer_rt_symbols(clang_rt.dfisan
    ARCHS ${arch}
    EXTRA dfisan.syms.extra
  )
  add_dependencies(dfisan
    clang_rt.dfisan-${arch}-symbols
  )
endforeach()