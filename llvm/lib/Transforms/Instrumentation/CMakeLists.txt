add_llvm_component_library(LLVMInstrumentation
  AddressSanitizer.cpp
  BoundsChecking.cpp
  CGProfile.cpp
  ControlHeightReduction.cpp
  DataFlowSanitizer.cpp
  GCOVProfiling.cpp
  MemProfiler.cpp
  MemorySanitizer.cpp
  IndirectCallPromotion.cpp
  Instrumentation.cpp
  InstrOrderFile.cpp
  InstrProfiling.cpp
  PGOInstrumentation.cpp
  PGOMemOPSizeOpt.cpp
  PoisonChecking.cpp
  SanitizerCoverage.cpp
  ValueProfileCollector.cpp
  ThreadSanitizer.cpp
  HWAddressSanitizer.cpp
  DataFlowIntegritySanitizer.cpp
  ReplaceWithSafeAlloc.cpp

  ADDITIONAL_HEADER_DIRS
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Transforms

  DEPENDS
  intrinsics_gen

  LINK_COMPONENTS
  Analysis
  Core
  MC
  Support
  TransformUtils
  ProfileData
  Svf
  )

add_definitions(-DENABLE_CFG)
target_include_directories(LLVMInstrumentation PRIVATE
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Analysis/SVF
)
