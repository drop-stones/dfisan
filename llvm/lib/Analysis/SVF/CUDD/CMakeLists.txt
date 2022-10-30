set(SOURCES
	epd.c
	st.c
	util.c
	mtr.c
	cuddInt.c
)

# add_llvm_library(Cudd ${SOURCES} LINK_LIBS m)
add_llvm_component_library(LLVMCudd
	${SOURCES}

	DEPENDS
	intrinsics_gen

	LINK_LIBS
	m 				# math libraries
)
target_include_directories(LLVMCudd PRIVATE ${LLVM_MAIN_INCLUDE_DIR}/llvm/Analysis/SVF)

set_target_properties(LLVMCudd PROPERTIES COMPILE_FLAGS "-Wno-all -Wno-extra -Wno-format -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -DHAVE_IEEE_754 -DSIZEOF_VOID_P=8 -DSIZEOF_LONG=8")
