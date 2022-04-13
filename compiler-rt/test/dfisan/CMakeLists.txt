# Settings for lit.site.cfg.py.in
set(DFISAN_LIT_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DFISAN_TESTSUITES)
set(DFISAN_TEST_ARCH ${DFISAN_SUPPORTED_ARCH})

foreach(arch ${DFISAN_TEST_ARCH})
  set(DFISAN_TEST_TARGET_ARCH ${arch})
  string(TOLOWER "-${arch}" DFISAN_TEST_CONFIG_SUFFIX)
  get_test_cc_for_arch(${arch} DFISAN_TEST_TARGET_CC DFISAN_TEST_TARGET_CFLAGS)
  string(TOUPPER ${arch} ARCH_UPPER_CASE)
  set(CONFIG_NAME ${ARCH_UPPER_CASE}Config)

  configure_lit_site_cfg(
    ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_NAME}/lit.site.cfg.py
  )
  list(APPEND DFISAN_TESTSUITES ${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_NAME})
endforeach()

set(DFISAN_TEST_DEPS ${SANITIZER_COMMON_LIT_TEST_DEPS})
if(NOT COMPILER_RT_STANDALONE_BUILD)
  list(APPEND DFISAN_TEST_DEPS dfisan)
endif()

add_lit_testsuite(check-dfisan "Running the DataFlowIntegritySanitizer tests"
  ${DFISAN_TESTSUITES}
  DEPENDS ${DFISAN_TEST_DEPS}
)
set_target_properties(check-dfisan PROPERTIES FOLDER "Compiler-RT Misc")