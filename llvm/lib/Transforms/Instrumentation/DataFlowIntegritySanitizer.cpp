#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Instrumentation/DataFlowIntegritySanitizer.h"
#include "UseDefAnalysis/UseDefAnalysisPass.h"
#include "UseDefAnalysis/UseDefChain.h"

using namespace llvm;
using namespace SVF;

PreservedAnalyses
DataFlowIntegritySanitizerPass::run(Module &M, ModuleAnalysisManager &MAM) {
  initializeSanitizerFuncs(M);

  auto Result = MAM.getResult<UseDefAnalysisPass>(M);
  SVF::UseDefChain *UseDef = Result.UseDef;

  IRBuilder<> Builder{M.getContext()};
  for (const auto *DefUsingPtr : UseDef->getDefUsingPtrList()) {
    insertDfiStoreFn(Builder, DefUsingPtr);
  }
  for (const auto &Iter : *UseDef) {
    const LoadSVFGNode *Use = Iter.first;
    SmallVector<Value *, 8> DefIDs;
    for (const auto *Def : Iter.second) {
      Value *DefID = ConstantInt::get(ArgTy, Def->getId(), false);
      DefIDs.push_back(DefID);
      insertDfiStoreFn(Builder, Def);
    }
    insertDfiLoadFn(Builder, Use, DefIDs);
  }

  return PreservedAnalyses::all();
}

void DataFlowIntegritySanitizerPass::initializeSanitizerFuncs(Module &M) {
  LLVMContext &Ctx = M.getContext();

  VoidTy = Type::getVoidTy(Ctx);
  ArgTy  = Type::getInt32Ty(Ctx);
  PtrTy  = Type::getInt32PtrTy(Ctx);

  SmallVector<Type *, 8> StoreArgTypes{PtrTy, ArgTy};
  FunctionType *StoreFnTy = FunctionType::get(VoidTy, StoreArgTypes, false);
  SmallVector<Type *, 8> LoadArgTypes{PtrTy, ArgTy};
  FunctionType *LoadFnTy = FunctionType::get(VoidTy, LoadArgTypes, true);

  DfiStoreFn = M.getOrInsertFunction("__dfisan_store_id", StoreFnTy);
  DfiLoadFn  = M.getOrInsertFunction("__dfisan_check_ids", LoadFnTy); // VarArg Function
}

/// Insert a DEF function after each store statement using pointer.
void DataFlowIntegritySanitizerPass::insertDfiStoreFn(IRBuilder<> &Builder, const StoreSVFGNode *StoreNode) {
  // Check whether the insertion is first time.
  const Instruction *Inst = StoreNode->getInst();
  const Instruction *NextInst = Inst->getNextNode();
  if (const CallInst *Call = dyn_cast<const CallInst>(NextInst)) {
    const Function *Callee = Call->getCalledFunction();
    if (Callee == nullptr || Callee->getName() == "__dfisan_store_id")
      return;
  }

  if (StoreInst *Store = dyn_cast<StoreInst>((Instruction *)Inst)) {
    Value *StoreAddr = Store->getPointerOperand();
    Value *DefID = ConstantInt::get(ArgTy, StoreNode->getId(), false);
    Builder.SetInsertPoint(Store->getNextNode());
    Builder.CreateCall(DfiStoreFn, {StoreAddr, DefID});
  }
}

/// Insert a CHECK function before each load statement.
void DataFlowIntegritySanitizerPass::insertDfiLoadFn(IRBuilder<> &Builder, const LoadSVFGNode *LoadNode, SmallVector<Value *, 8> &DefIDs) {
  // Check whether the insertion is first time.
  const Instruction *Inst = LoadNode->getInst();
  const Instruction *NextInst = Inst->getNextNode();
  if (const CallInst *Call = dyn_cast<const CallInst>(NextInst)) {
    const Function *Callee = Call->getCalledFunction();
    if (Callee == nullptr || Callee->getName() == "__dfisan_check_ids")
      return;
  }

  if (LoadInst *Load = dyn_cast<LoadInst>((Instruction *)Inst)) {
    Value *LoadAddr = Load->getPointerOperand();
    Value *UseID = ConstantInt::get(ArgTy, LoadNode->getId(), false);
    Value *Argc  = ConstantInt::get(ArgTy, DefIDs.size(), false);
    SmallVector<Value *, 8> Args{LoadAddr, Argc};
    Args.append(DefIDs);
    Builder.SetInsertPoint(Load);
    Builder.CreateCall(DfiLoadFn, Args);
  }
}