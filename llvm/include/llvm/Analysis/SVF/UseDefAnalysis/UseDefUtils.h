
#ifndef LLVM_ANALYSIS_SVF_USEDEFANALYSIS_USEDEFUTILS_H
#define LLVM_ANALYSIS_SVF_USEDEFANALYSIS_USEDEFUTILS_H

#include "Graphs/SVFG.h"
#include "Util/SVFModule.h"

using namespace llvm;
using namespace SVF;

std::string getModuleName(Module &M);
std::string getModuleName(SVFModule *M);

bool isMemcpy(const StmtSVFGNode *Node);
bool isMemset(const StmtSVFGNode *Node);
bool isMemcpyOrMemset(const StmtSVFGNode *Node);

#endif