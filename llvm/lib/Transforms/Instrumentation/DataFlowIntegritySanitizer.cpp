#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/TypeFinder.h"
#include "llvm/Transforms/Instrumentation/DataFlowIntegritySanitizer.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"  /* appendToUsed */
#include "llvm/Support/Debug.h"

#include "dg/Passes/UseDefAnalysisPass.h"
#include "dg/Passes/UseDefBuilder.h"

#define DEBUG_TYPE "dfi-instrument"

using namespace llvm;

constexpr char DfisanModuleCtorName[] = "dfisan.module_ctor";
constexpr char DfisanInitFnName[]     = "__dfisan_init";

constexpr char CommonDfisanStoreFnName[] = "__dfisan_store_id";
constexpr char DfisanStoreNFnName[]      = "__dfisan_store_id_n";
constexpr char DfisanStore1FnName[]      = "__dfisan_store_id_1";
constexpr char DfisanStore2FnName[]      = "__dfisan_store_id_2";
constexpr char DfisanStore4FnName[]      = "__dfisan_store_id_4";
constexpr char DfisanStore8FnName[]      = "__dfisan_store_id_8";
constexpr char DfisanStore16FnName[]     = "__dfisan_store_id_16";

constexpr char CommonDfisanLoadFnName[]  = "__dfisan_check_ids";
constexpr char DfisanLoadNFnName[]       = "__dfisan_check_ids_n";
constexpr char DfisanLoad1FnName[]       = "__dfisan_check_ids_1";
constexpr char DfisanLoad2FnName[]       = "__dfisan_check_ids_2";
constexpr char DfisanLoad4FnName[]       = "__dfisan_check_ids_4";
constexpr char DfisanLoad8FnName[]       = "__dfisan_check_ids_8";
constexpr char DfisanLoad16FnName[]      = "__dfisan_check_ids_16";

PreservedAnalyses
DataFlowIntegritySanitizerPass::run(Module &M, ModuleAnalysisManager &MAM) {
  auto &Result = MAM.getResult<UseDefAnalysisPass>(M);
  UseDef = Result.getBuilder();
  this->M = &M;
  Builder = std::make_unique<IRBuilder<>>(M.getContext());

  initializeSanitizerFuncs();
  insertDfiInitFn();

  for (auto DI = UseDef->def_begin(), DE = UseDef->def_end(); DI != DE; DI++) {
    insertDfiStoreFn((*DI)->getValue());
  }
  for (auto UI = UseDef->use_begin(), UE = UseDef->use_end(); UI != UE; UI++) {
    auto *Use = (*UI)->getValue();
    SmallVector<Value *, 8> DefIDs;
    for (auto *Def : UseDef->getDDA()->getLLVMDefinitions(Use)) {
      Value *DefID = ConstantInt::get(ArgTy, UseDef->getDefID(Def), false);
      DefIDs.push_back(DefID);
    }
    insertDfiLoadFn(Use, DefIDs);
  }

  return PreservedAnalyses::none();
}

void DataFlowIntegritySanitizerPass::initializeSanitizerFuncs() {
  LLVMContext &Ctx = M->getContext();

  int LongSize = M->getDataLayout().getPointerSizeInBits();
  PtrTy  = Type::getIntNTy(Ctx, LongSize);
  VoidTy = Type::getVoidTy(Ctx);
  ArgTy  = Type::getInt16Ty(Ctx);
  Int8Ty = Type::getInt8Ty(Ctx);
  Int32Ty = Type::getInt32Ty(Ctx);

  SmallVector<Type *, 8> StoreArgTypes{PtrTy, ArgTy};
  FunctionType *StoreFnTy = FunctionType::get(VoidTy, StoreArgTypes, false);
  SmallVector<Type *, 8> StoreNArgTypes{PtrTy, Int32Ty, ArgTy};
  FunctionType *StoreNFnTy = FunctionType::get(VoidTy, StoreNArgTypes, false);
  SmallVector<Type *, 8> LoadArgTypes{PtrTy, ArgTy};
  FunctionType *LoadFnTy = FunctionType::get(VoidTy, LoadArgTypes, true);

  DfiInitFn  = M->getOrInsertFunction(DfisanInitFnName, VoidTy);
  DfiStoreNFn = M->getOrInsertFunction(DfisanStoreNFnName, StoreNFnTy);
  DfiLoadNFn  = M->getOrInsertFunction(DfisanLoadNFnName, LoadFnTy); // VarArg Function

  DfiStore1Fn = M->getOrInsertFunction(DfisanStore1FnName, StoreFnTy);
  DfiStore2Fn = M->getOrInsertFunction(DfisanStore2FnName, StoreFnTy);
  DfiStore4Fn = M->getOrInsertFunction(DfisanStore4FnName, StoreFnTy);
  DfiStore8Fn = M->getOrInsertFunction(DfisanStore8FnName, StoreFnTy);
  DfiStore16Fn = M->getOrInsertFunction(DfisanStore16FnName, StoreFnTy);

  DfiLoad1Fn = M->getOrInsertFunction(DfisanLoad1FnName, LoadFnTy);
  DfiLoad2Fn = M->getOrInsertFunction(DfisanLoad2FnName, LoadFnTy);
  DfiLoad4Fn = M->getOrInsertFunction(DfisanLoad4FnName, LoadFnTy);
  DfiLoad8Fn = M->getOrInsertFunction(DfisanLoad8FnName, LoadFnTy);
  DfiLoad16Fn = M->getOrInsertFunction(DfisanLoad16FnName, LoadFnTy);
}

/// Insert a constructor function in comdat
void DataFlowIntegritySanitizerPass::insertDfiInitFn() {
  LLVM_DEBUG(dbgs() << __func__ << "\n");

  // Create Sanitizer Ctor
  Function *Ctor = Function::createWithDefaultAttr(
    FunctionType::get(VoidTy, false),
    GlobalValue::InternalLinkage, 0, DfisanModuleCtorName, M);
  Ctor->addFnAttr(Attribute::NoUnwind);
  BasicBlock *CtorBB = BasicBlock::Create(M->getContext(), "", Ctor);
  ReturnInst::Create(M->getContext(), CtorBB);

  // Insert Init Function
  Builder->SetInsertPoint(Ctor->getEntryBlock().getTerminator());
  Builder->CreateCall(DfiInitFn, {});

  // Ensure Ctor cannot be discarded, even if in a comdat.
  appendToUsed(*M, {Ctor});
  // Put the constructor in GlobalCtors
  appendToGlobalCtors(*M, Ctor, 1);

  // Insert DfiStoreFn for GlobalInit
/*
  for (const auto &GlobalInit : UseDef->getGlobalInitList()) {
    const Value *Val = GlobalInit->getValue();
    assert(Val != nullptr);
    LLVM_DEBUG(dbgs() << "GlobalInit: " << *Val << "\n");
    const auto DefVars = GlobalInit->getDefSVFVars();
    for (const auto DefVarID : DefVars) {
      if (UseDef->containsOffsetVector(GlobalInit)) {   // Global struct initialization
        createDfiStoreFnForAggregateData(M, Builder, GlobalInit);
      } else {  // Global primitive data initialization
        const auto *DstNode = Svfg->getPAG()->getObject(DefVarID);
        Value *StorePointer = (Value *)DstNode->getValue();
        if (StorePointer == nullptr)  // True if string literal or struct init values
          continue;
        createDfiStoreFn(M, Builder, GlobalInit, StorePointer, Builder.GetInsertBlock()->getTerminator());
      }
    }
  }
*/
}

/// Insert a DEF function after each store statement using pointer.
void DataFlowIntegritySanitizerPass::insertDfiStoreFn(Value *Def) {
  if (Instruction *DefInst = dyn_cast<Instruction>(Def)) {
    if (StoreInst *Store = dyn_cast<StoreInst>(DefInst)) {
      auto *StoreTarget = Store->getPointerOperand();
      unsigned Size = M->getDataLayout().getTypeStoreSize(StoreTarget->getType());
      createDfiStoreFn(UseDef->getDefID(Store), StoreTarget, Size, Store->getNextNode());
    } else if (llvm::isa<MemCpyInst>(DefInst) || llvm::isa<MemSetInst>(DefInst)) {
      // createDfiStoreFn()
    } else {
      // assert(false && "No support Def");
    }
  } else {
    // assert(false && "No support Def");
  }
}

/// Insert a CHECK function before each load statement.
void DataFlowIntegritySanitizerPass::insertDfiLoadFn(Value *Use, SmallVector<Value *, 8> &DefIDs) {
  if (Instruction *UseInst = dyn_cast<Instruction>(Use)) {
    if (LoadInst *Load = dyn_cast<LoadInst>(UseInst)) {
      auto *LoadTarget = Load->getPointerOperand();
      unsigned Size = M->getDataLayout().getTypeStoreSize(LoadTarget->getType());
      createDfiLoadFn(LoadTarget, Size, DefIDs, Load);
    }
  }
}

/// Create a function call to DfiStoreFn.
void DataFlowIntegritySanitizerPass::createDfiStoreFn(dg::DefID DefID, Value *StoreTarget, unsigned Size) {
  Value *SizeVal = ConstantInt::get(Int32Ty, Size, false);
  Value *StoreAddr = Builder->CreatePtrToInt(StoreTarget, PtrTy);
  Value *DefIDVal = ConstantInt::get(ArgTy, DefID);

  switch(Size) {
  case 1:   Builder->CreateCall(DfiStore1Fn, {StoreAddr, DefIDVal});  break;
  case 2:   Builder->CreateCall(DfiStore2Fn, {StoreAddr, DefIDVal});  break;
  case 4:   Builder->CreateCall(DfiStore4Fn, {StoreAddr, DefIDVal});  break;
  case 8:   Builder->CreateCall(DfiStore8Fn, {StoreAddr, DefIDVal});  break;
  case 16:  Builder->CreateCall(DfiStore16Fn,{StoreAddr, DefIDVal});  break;
  default:  Builder->CreateCall(DfiStoreNFn, {StoreAddr, SizeVal, DefIDVal});  break;
  }
}

void DataFlowIntegritySanitizerPass::createDfiStoreFn(dg::DefID DefID, Value *StoreTarget, unsigned Size, Instruction *InsertPoint) {
  Builder->SetInsertPoint(InsertPoint);
  createDfiStoreFn(DefID, StoreTarget, Size);
}


/// Create a function call to DfiLoadFn.
void DataFlowIntegritySanitizerPass::createDfiLoadFn(Value *LoadTarget, unsigned Size, SmallVector<Value *, 8> &DefIDs) {
  Value *SizeVal = ConstantInt::get(ArgTy, Size, false);
  Value *LoadAddr = Builder->CreatePtrToInt(LoadTarget, PtrTy);
  Value *Argc = ConstantInt::get(ArgTy, DefIDs.size(), false);

  SmallVector<Value *, 8> Args{LoadAddr, Argc};
  Args.append(DefIDs);

  switch(Size) {
  case 1:   Builder->CreateCall(DfiLoad1Fn, Args);   break;
  case 2:   Builder->CreateCall(DfiLoad2Fn, Args);   break;
  case 4:   Builder->CreateCall(DfiLoad4Fn, Args);   break;
  case 8:   Builder->CreateCall(DfiLoad8Fn, Args);   break;
  case 16:  Builder->CreateCall(DfiLoad16Fn,Args);   break;
  default:  auto *Iter = Args.begin() + 1;
            Args.insert(Iter, SizeVal);
            Builder->CreateCall(DfiLoadNFn, Args);   break;
  }
}

void DataFlowIntegritySanitizerPass::createDfiLoadFn(Value *LoadTarget, unsigned Size, SmallVector<Value *, 8> &DefIDs, Instruction *InsertPoint) {
  Builder->SetInsertPoint(InsertPoint);
  createDfiLoadFn(LoadTarget, Size, DefIDs);
}

/*
void DataFlowIntegritySanitizerPass::createDfiStoreFnForAggregateData(Value *Store, Instruction *InsertPoint) {
  Builder.SetInsertPoint(InsertPoint);
  createDfiStoreFnForAggregateData(M, Builder, StoreNode);
}
*/

/*
void DataFlowIntegritySanitizerPass::createDfiStoreFnForAggregateData(Module &M, llvm::IRBuilder<> &Builder, const StoreVFGNode *StoreNode) {
  const auto &OffsetVec = UseDef->getOffsetVector(StoreNode);
  assert(OffsetVec.size() != 0);
  if (llvm::isa<StructOffset>(OffsetVec[0])) {
    Value *FieldAddr = createStructGep(Builder, StoreNode, OffsetVec);
    createDfiStoreFn(M, Builder, StoreNode, FieldAddr);
  } else if (llvm::isa<ArrayOffset>(OffsetVec[0])) {
    Value *ArrayAddr = (Value *)OffsetVec[0]->Base;
    createDfiStoreFn(M, Builder, StoreNode, ArrayAddr);
  } else if (llvm::isa<PointerOffset>(OffsetVec[0])) {
    // TODO
    PointerOffset *Offset = llvm::dyn_cast<PointerOffset>(OffsetVec[0]);
    Value *BaseAddr = (Value *)Offset->Base;
    Value *Length = (Value *)Offset->Length;
    createDfiStoreFn(M, Builder, StoreNode, BaseAddr, Length);
  } else {
    assert(false && "Invalid Offset Type!!");
  }
}

Value *
DataFlowIntegritySanitizerPass::createStructGep(llvm::IRBuilder<> &Builder, const StoreSVFGNode *StoreNode, const std::vector<struct FieldOffset *> &OffsetVec) {
  assert(OffsetVec.size() != 0 && llvm::isa<StructOffset>(OffsetVec[0]));
  Value *CurVal = (Value *)OffsetVec[0]->Base;
  for (const auto *Offset : OffsetVec) {
    const auto *StructOff = llvm::dyn_cast<StructOffset>(Offset);
    LLVM_DEBUG(dbgs() << "Type: " << *StructOff->StructTy << "\n");
    LLVM_DEBUG(dbgs() << "Value: " << *CurVal << "\n");
    CurVal = Builder.CreateStructGEP((Type *)StructOff->StructTy, CurVal, StructOff->Offset);
  }
  return CurVal;
///*
  const auto *Head = OffsetVec[0];
  Value *CurVal = (Value *)Head->Base;
  if (const auto *StructOff = dyn_cast<StructOffset>(Head)) {
    for (auto *Offset : OffsetVec) {
      const auto *EleStructOff = dyn_cast<StructOffset>(Offset);
      LLVM_DEBUG(dbgs() << "Type: " << *EleStructOff->StructTy << "\n");
      LLVM_DEBUG(dbgs() << "Value: " << *CurVal << "\n");
      CurVal = Builder.CreateStructGEP((Type *)EleStructOff->StructTy, CurVal, EleStructOff->Offset);
    }
  } else if (const auto *ArrayOff = dyn_cast<ArrayOffset>(Head)) {
    // do nothing
  } else if (const auto *PointerOff = dyn_cast<PointerOffset>(Head)) {
    // cast to [i8 * Length]
  }
  for (auto *Offset : OffsetVec) {
    if (auto *StructOff = dyn_cast<StructOffset>(Offset)) {
      LLVM_DEBUG(dbgs() << "Type: " << *StructOff->StructTy << "\n");
      LLVM_DEBUG(dbgs() << "Value: " << *CurVal << "\n");
      CurVal = Builder.CreateStructGEP((Type *)StructOff->StructTy, CurVal, StructOff->Offset);
    }
  }
}
*/
