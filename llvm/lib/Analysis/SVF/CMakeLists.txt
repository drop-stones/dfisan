add_subdirectory(CUDD)

configure_file(config.in ${LLVM_MAIN_INCLUDE_DIR}/llvm/Analysis/SVF/Util/config.h @ONLY)

# Due to a mutual dependencies, all the sub projects of the SVG are merged here
# Otherwise it is impossible to load the dependencies in opt
# NOTE: if the SVF should be linked into opt, we should probably use the individual sub projects here, rather than the combined project 

file (GLOB SOURCES
        SVF-FE/*.cpp
        Graphs/*.cpp
        Util/*.cpp
        MemoryModel/*.cpp
        MSSA/*.cpp
        WPA/*.cpp
        MTA/*.cpp
        SABER/*.cpp
        DDA/*.cpp
        CFL/*.cpp
        FastCluster/*.cpp
        Dfisan/*.cpp
        ModulePass/*.cpp
)

add_llvm_component_library(LLVMSvf
    ${SOURCES}

    DEPENDS
    intrinsics_gen

    LINK_LIBS
    pthread
    ${Z3_LIBRARIES}

    LINK_COMPONENTS
    Cudd
    BitWriter
    Core
    IRReader
    InstCombine
    Target
    Linker
    Analysis
    ScalarOpts
    Support
    TransformUtils
)
target_include_directories(LLVMSvf PRIVATE
    ${LLVM_MAIN_INCLUDE_DIR}/llvm/Analysis/SVF
    ${Z3_INCLUDE_DIR}
)
target_link_directories(LLVMSvf PRIVATE ${CMAKE_BINARY_DIR}/lib/Cudd)

# sqlite3 settings
get_target_property(LLVMSqlite3_INCLUDES LLVMSqlite3 INCLUDE_DIRECTORIES)
target_include_directories(LLVMSvf PRIVATE ${LLVMSqlite3_INCLUDES})
target_link_libraries(LLVMSvf PRIVATE LLVMSqlite3)
