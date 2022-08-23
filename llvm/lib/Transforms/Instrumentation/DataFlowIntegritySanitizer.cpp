#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/TypeFinder.h"
#include "llvm/Transforms/Instrumentation/DataFlowIntegritySanitizer.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"  /* appendToUsed */
#include "llvm/Support/Debug.h"

#include "dg/Passes/UseDefAnalysisPass.h"
#include "dg/Passes/UseDefBuilder.h"
#include "dg/Passes/DfiUtils.h"

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
  const auto *DDA = UseDef->getDDA();

  initializeSanitizerFuncs();
  insertDfiInitFn();

  if (UseDef->isSelectiveDfi()) {
    for (auto *Def : UseDef->getProtectInfo().Defs) {
      insertDfiStoreFn((Value *)Def);
    }
    for (auto *Use : UseDef->getProtectInfo().Uses) {
      // Skip unknown value uses (e.g., argv[]).
      const auto &UseSites = DDA->getNode(Use)->getUses();
      if (UseSites.size() == 1 && DDA->getValue(UseSites.begin()->target) == nullptr) {
        llvm::errs() << "Skip Use: " << *Use << "\n";
        continue;
      }

      SmallVector<Value *, 8> DefIDs;
      for (auto *Def : UseDef->getDDA()->getLLVMDefinitions((Value *)Use)) {
        Value *DefID = ConstantInt::get(ArgTy, UseDef->getDefID(Def), false);
        DefIDs.push_back(DefID);
      }
      insertDfiLoadFn((Value *)Use, DefIDs);
    }
  } else {
    for (auto DI = UseDef->def_begin(), DE = UseDef->def_end(); DI != DE; DI++) {
      insertDfiStoreFn((*DI)->getValue());
    }
    for (auto UI = UseDef->use_begin(), UE = UseDef->use_end(); UI != UE; UI++) {
      auto *Use = (*UI)->getValue();

      // Skip unknown value uses (e.g., argv[]).
      const auto &UseSites = DDA->getNode(Use)->getUses();
      if (UseSites.size() == 1 && DDA->getValue(UseSites.begin()->target) == nullptr) {
        llvm::errs() << "Skip Use: " << *Use << "\n";
        continue;
      }

      SmallVector<Value *, 8> DefIDs;
      for (auto *Def : UseDef->getDDA()->getLLVMDefinitions(Use)) {
        Value *DefID = ConstantInt::get(ArgTy, UseDef->getDefID(Def), false);
        DefIDs.push_back(DefID);
      }
      insertDfiLoadFn(Use, DefIDs);
    }
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
  Int64Ty = Type::getInt64Ty(Ctx);

  SmallVector<Type *, 8> StoreArgTypes{PtrTy, ArgTy};
  FunctionType *StoreFnTy = FunctionType::get(VoidTy, StoreArgTypes, false);
  SmallVector<Type *, 8> StoreNArgTypes{PtrTy, Int64Ty, ArgTy};
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
  if (UseDef->isSelectiveDfi()) {
    for (auto *GlobVal : UseDef->getProtectInfo().Globals) {
      insertDfiStoreFn((Value *)GlobVal);
    }
  } else {
    for (auto GI = UseDef->glob_begin(); GI != UseDef->glob_end(); GI++) {
      auto *GlobVal = (llvm::Value *)UseDef->getDDA()->getValue(*GI);
      if (auto *GVal = llvm::dyn_cast<GlobalVariable>(GlobVal)) {
        if (GVal->isConstant() || GVal->getName() == "llvm.global.annotations") { // Skip constant or annotations
          llvm::errs() << "Skip GlobalVal: " << *GlobVal << "\n";
          continue;
        }
      }
      insertDfiStoreFn(GlobVal);
    }
  }
}

/// Insert a DEF function after each store statement using pointer.
void DataFlowIntegritySanitizerPass::insertDfiStoreFn(Value *Def) {
  llvm::errs() << "InsertDfiStoreFn: " << *Def << "\n";

  if (Instruction *DefInst = dyn_cast<Instruction>(Def)) {
    if (StoreInst *Store = dyn_cast<StoreInst>(DefInst)) {
      auto *StoreTarget = Store->getPointerOperand();
      unsigned Size = M->getDataLayout().getTypeStoreSize(StoreTarget->getType()->getNonOpaquePointerElementType());
      createDfiStoreFn(UseDef->getDefID(Store), StoreTarget, Size, Store->getNextNode());
    } else if (MemCpyInst *Memcpy = dyn_cast<MemCpyInst>(DefInst)) {
      auto *StoreTarget = Memcpy->getOperand(0);
      auto *SizeVal = Memcpy->getOperand(2);
      createDfiStoreFn(UseDef->getDefID(Memcpy), StoreTarget, SizeVal, Memcpy->getNextNode());
    } else if (MemSetInst *Memset = dyn_cast<MemSetInst>(DefInst)) {
      auto *StoreTarget = Memset->getOperand(0);
      auto *SizeVal = Memset->getOperand(2);
      createDfiStoreFn(UseDef->getDefID(Memset), StoreTarget, SizeVal, Memset->getNextNode());
    } else if (CallInst *Call = dyn_cast<CallInst>(DefInst)) {
      auto *Callee = Call->getCalledFunction();
      if (Callee->getName() == "calloc") {
        auto *Nmem = Call->getOperand(0);
        auto *Size = Call->getOperand(1);
        Builder->SetInsertPoint(Call->getNextNode());
        auto *SizeVal = Builder->CreateNUWMul(Nmem, Size);
        createDfiStoreFn(UseDef->getDefID(Call), Def, SizeVal);
      } else if (Callee->getName() == "fgets") {
        auto *StoreTarget = Call->getOperand(0);
        auto *SizeVal = Call->getOperand(1);
        createDfiStoreFn(UseDef->getDefID(Call), StoreTarget, SizeVal, Call->getNextNode());
      } else if (Callee->getName() == "__isoc99_sscanf") {
        for (unsigned Idx = 2; Idx < Call->arg_size(); Idx++) {
          auto *StoreTarget = Call->getOperand(Idx);
          unsigned Size = M->getDataLayout().getTypeStoreSize(StoreTarget->getType()->getNonOpaquePointerElementType());
          createDfiStoreFn(UseDef->getDefID(Call), StoreTarget, Size, Call->getNextNode());
        }
      } else if (Callee->getName() == "read") {
        auto *StoreTarget = Call->getOperand(1);
        auto *SizeVal = Def;
        llvm::errs() << "Instrument " << Callee->getName() << "\n";
        llvm::errs() << " - Target: " << *StoreTarget << ", Size: " << *SizeVal << "\n";
        createDfiStoreFn(UseDef->getDefID(Call), StoreTarget, SizeVal, Call->getNextNode());
      } else {
        llvm::errs() << "No support Def function: " << Callee->getName() << "\n";
      }
    } else if (AllocaInst *Alloca = dyn_cast<AllocaInst>(Def)) {
      // Do nothing
    } else {
      llvm::errs() << "No support DefInst: " << *DefInst << "\n";
      // assert(false && "No support Def");
    }
  } else if (GlobalVariable *GlobVar = dyn_cast<GlobalVariable>(Def)) {
    unsigned Size = M->getDataLayout().getTypeStoreSize(GlobVar->getType()->getNonOpaquePointerElementType());
    createDfiStoreFn(UseDef->getDefID(GlobVar), GlobVar, Size, Builder->GetInsertBlock()->getTerminator());
  } else {
    llvm::errs() << "No support Def: " << *Def << "\n";
    // assert(false && "No support Def");
  }
}

/// Insert a CHECK function before each load statement.
void DataFlowIntegritySanitizerPass::insertDfiLoadFn(Value *Use, SmallVector<Value *, 8> &DefIDs) {
  llvm::errs() << "InsertDfiLoadFn: " << *Use << "\n";

  if (Instruction *UseInst = dyn_cast<Instruction>(Use)) {
    if (LoadInst *Load = dyn_cast<LoadInst>(UseInst)) {
      auto *LoadTarget = Load->getPointerOperand();
      unsigned Size = M->getDataLayout().getTypeStoreSize(LoadTarget->getType()->getNonOpaquePointerElementType());
      createDfiLoadFn(LoadTarget, Size, DefIDs, Load);
    }
  }
}

/// Create a function call to DfiStoreFn.
void DataFlowIntegritySanitizerPass::createDfiStoreFn(dg::DefID DefID, Value *StoreTarget, unsigned Size, Instruction *InsertPoint) {
  Value *SizeVal = ConstantInt::get(Int64Ty, Size, false);
  createDfiStoreFn(DefID, StoreTarget, SizeVal, InsertPoint);
}

void DataFlowIntegritySanitizerPass::createDfiStoreFn(dg::DefID DefID, Value *StoreTarget, Value *SizeVal, Instruction *InsertPoint) {
  if (InsertPoint != nullptr)
    Builder->SetInsertPoint(InsertPoint);
  unsigned Size = llvm::isa<ConstantInt>(SizeVal) ? llvm::cast<ConstantInt>(SizeVal)->getZExtValue() : 0;
  Value *SizeArg = SizeVal;
  if (SizeVal->getType() != Int64Ty) {
    if (llvm::isa<ConstantInt>(SizeVal))
      SizeArg = ConstantInt::get(Int64Ty, Size);
    else
      SizeArg = Builder->CreateBitCast(SizeVal, Int64Ty);
  }
  Value *StoreAddr = Builder->CreatePtrToInt(StoreTarget, PtrTy);
  Value *DefIDVal = ConstantInt::get(ArgTy, DefID);

  switch(Size) {
  case 1:   Builder->CreateCall(DfiStore1Fn, {StoreAddr, DefIDVal});  break;
  case 2:   Builder->CreateCall(DfiStore2Fn, {StoreAddr, DefIDVal});  break;
  case 4:   Builder->CreateCall(DfiStore4Fn, {StoreAddr, DefIDVal});  break;
  case 8:   Builder->CreateCall(DfiStore8Fn, {StoreAddr, DefIDVal});  break;
  case 16:  Builder->CreateCall(DfiStore16Fn,{StoreAddr, DefIDVal});  break;
  default:  Builder->CreateCall(DfiStoreNFn, {StoreAddr, SizeArg, DefIDVal});  break;
  }
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
