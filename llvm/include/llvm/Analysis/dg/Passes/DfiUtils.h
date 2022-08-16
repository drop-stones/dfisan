#ifndef LLVM_ANALYSIS_DG_PASSES_DFIUTILS_H
#define LLVM_ANALYSIS_DG_PASSES_DFIUTILS_H

#include <llvm/IR/Value.h>
#include "dg/llvm/DataDependence/DataDependence.h"

using namespace llvm;

namespace dg {

void printUseDefFromDebugLoc(dda::LLVMDataDependenceAnalysis *DDA, Value *Val, unsigned int Col, unsigned int Line);
void printDefinitionsFromDebugLoc(dda::RWNode *Use, std::vector<dda::RWNode *> &Defs, unsigned int Col, unsigned int Line);
void printRWNode(dda::LLVMDataDependenceAnalysis *DDA, dda::RWNode *Node);
void printRWNodeVector(std::vector<dda::RWNode *> &Nodes, const dda::LLVMReadWriteGraphBuilder *RWBuilder = nullptr, dda::ReadWriteGraph *Graph = nullptr);
void printRWBBlock(dda::RWBBlock *Block, const dda::LLVMReadWriteGraphBuilder *RWBuilder);

} // namespace dg

#endif
