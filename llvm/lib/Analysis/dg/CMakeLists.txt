# Initial settings
add_definitions(-DENABLE_CFG)

file (GLOB SOURCES
	# dganalysis
	Offset.cpp
	Debug.cpp
	BBlockBase.cpp

	# dgpta
	PointerAnalysis/*.cpp

	# dgdda
	ReadWriteGraph/ReadWriteGraph.cpp
	MemorySSA/*.cpp

	# dgcda
	ControlDependence/NTSCD.cpp

	# dgsdg
	SystemDependenceGraph/DependenceGraph.cpp

	# dgvra
	ValueRelations/Relations.cpp

	# dgllvmpta
	llvm/PointerAnalysis/*.cpp

	# dgllvmforkjoin
	llvm/ForkJoin/ForkJoin.cpp

	# dgllvmdda
	llvm/ReadWriteGraph/*.cpp
	llvm/DataDependenceAnalysis/LLVMDataDependenceAnalysis.cpp

	# dgllvmthreadregions
	llvm/ThreadRegions/*.cpp
	llvm/ThreadRegions/Nodes/*.cpp
	llvm/ThreadRegions/Graphs/*.cpp

	# dgllvmcda
	llvm/ControlDependence/*.cpp
	llvm/ControlDependence/legacy/*.cpp

	# dgllvmdg
	llvm/LLVMNode.cpp
	llvm/LLVMDependenceGraph.cpp
	llvm/LLVMDGVerifier.cpp
	llvm/Dominators/PostDominators.cpp
	llvm/DefUse/DefUse.cpp

	# dgllvmvra
	llvm/ValueRelations/*.cpp

	# dgllvmsdg
	llvm/SystemDependenceGraph/*.cpp
)

add_llvm_component_library(LLVMdg
	${SOURCES}

	LINK_COMPONENTS
	Core
	Analysis
	BitWriter
	IRReader
	Support
)
target_include_directories(LLVMdg PRIVATE
	${LLVM_MAIN_INCLUDE_DIR}/llvm/Analysis
)

# sqlite3 settings
get_target_property(LLVMSqlite3_INCLUDES LLVMSqlite3 INCLUDE_DIRECTORIES)
target_include_directories(LLVMSvf PRIVATE ${LLVMSqlite3_INCLUDES})
target_link_libraries(LLVMSvf PRIVATE LLVMSqlite3)
