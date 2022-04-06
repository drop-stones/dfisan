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

  IRBuilder<> Builder{M.getContext()};
  for (const auto &Iter : *Result.UseDef) {
    const LoadSVFGNode *Use = Iter.first;
    if (LoadInst *Load = dyn_cast<LoadInst>((Instruction *)Use->getInst())) {
      Value *LoadAddr = Load->getPointerOperand();
      Value *UseID = ConstantInt::get(ArgTy, Use->getId(), false);
      SmallVector<Value *, 8> Args{LoadAddr};
      for (const auto *Def : Iter.second) {
        // Insert a DEF function after store statement.
        if (StoreInst *Store = dyn_cast<StoreInst>((Instruction *)Def->getInst())) {
          Value *StoreAddr = Store->getPointerOperand();
          Value *DefID = ConstantInt::get(ArgTy, Def->getId(), false);
          Builder.SetInsertPoint(Store->getNextNode());
          Builder.CreateCall(DfiStoreFn, {StoreAddr, DefID});

          Args.push_back(DefID);
        }
      }
      // Insert a CHECK function before load statement.
      Builder.SetInsertPoint(Load);
      Builder.CreateCall(DfiLoadFn, Args);
    }
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

  DfiStoreFn = M.getOrInsertFunction("__dfi_store_id", StoreFnTy);
  DfiLoadFn  = M.getOrInsertFunction("__dfi_check_ids", LoadFnTy); // VarArg Function
}