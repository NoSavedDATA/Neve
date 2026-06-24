
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm-c/TargetMachine.h"

#include "llvm/MC/TargetRegistry.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

// #include "../../lsp/json.hpp"
#include "../include.h"

ExitOnError ExitOnErr;
std::unordered_map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;
std::unordered_map<std::string, PrototypeAST*> PriorityProtos;
std::unordered_map<std::string, std::function<llvm::Type*(std::unique_ptr<LLVMContext>&)>> data_register_fn;
std::unordered_map<std::string, std::function<llvm::PointerType*(std::unique_ptr<LLVMContext>&)>> data_ptr_register_fn;

inline void RegisterData() {
    for (auto data_info : data_register_fn) {
        std::string name = data_info.first;
        llvm::Type *ty = data_info.second(TheContext);
        str_toTy[name] = ty;
    }
}

std::string EmitPtx(bool re_emit) {

    static std::string ptx = "";
    if (ptx==""||re_emit) {
        llvm::SmallString<0> Buffer;
        llvm::raw_svector_ostream OS(Buffer);

        llvm::legacy::PassManager PM;

        if (PtxTM->addPassesToEmitFile(
                PM,
                OS,
                nullptr,
                llvm::CodeGenFileType::AssemblyFile)) {
            std::cout << "CANNOT EMIT PTX" << "\n";
            std::exit(0);
        }

        PM.run(*PtxModule);

        // OS.flush();
        std::string PTX(Buffer.str());
        ptx = PTX;
    }

    return ptx;
}

void InitializeModule() {
  // Open a new context and module.
  TheContext = std::make_unique<LLVMContext>();
  TheModule = std::make_unique<Module>("my cool jit", *TheContext);
  CurModule = TheModule.get();
  PtxModule = std::make_unique<Module>("neve_gpu", *TheContext);
  if (IsJIT) { // As jit
      TheModule->setDataLayout(TheJIT->getDataLayout());
  }
  else { // As compiler
      char *triple_c = LLVMGetDefaultTargetTriple();
      std::string TargetTriple(triple_c);
      LLVMDisposeErrorMessage(triple_c);

      std::string Error;
      auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);

      TargetOptions opt;
      auto RM = std::optional<Reloc::Model>();

      CTM = std::unique_ptr<TargetMachine>(
        Target->createTargetMachine(TargetTriple, "x86-64-v3", "+sse2", opt, RM)
    );
  }

  {
    std::string Error;
    auto Target = llvm::TargetRegistry::lookupTarget("nvptx64", Error);

    llvm::TargetOptions Opt;
    auto RM = std::optional<llvm::Reloc::Model>();

    PtxTM = std::unique_ptr<llvm::TargetMachine>(
        Target->createTargetMachine(
            "nvptx64-nvidia-cuda",
            "sm_80",          // or sm_70, sm_86, etc
            "",
            Opt,
            RM
        )
    );
    PtxModule->setDataLayout(PtxTM->createDataLayout());
    PtxModule->setTargetTriple("nvptx64-nvidia-cuda");
  }
  //std::cout << "Initialize Module\n";

  // Create a new builder for the module.
  Builder = std::make_unique<IRBuilder<>>(*TheContext);

  floatPtrTy = Type::getFloatTy(*TheContext)->getPointerTo();
  int8PtrTy = Type::getInt8Ty(*TheContext)->getPointerTo();
  int8Ty = Type::getInt8Ty(*TheContext);
  intTy = Type::getInt32Ty(*TheContext);
  int16Ty = Type::getInt16Ty(*TheContext);
  int64Ty = Type::getInt64Ty(*TheContext);
  floatTy = Type::getFloatTy(*TheContext);
  boolTy = Type::getInt1Ty(*TheContext);
  voidTy = Type::getVoidTy(*TheContext);
  ShallCodegen = true;
  seen_var_attr = false;
  tuple_cache.clear();

  Generate_Struct_Types();
  str_toTy = {{"char", int8Ty}, {"i8", int8Ty}, {"int", intTy}, {"i64", int64Ty}, {"i16", int16Ty},
              {"bool", boolTy}, {"float_ptr", floatPtrTy}, {"void", voidTy},
              {"float", floatTy}, {"void", voidTy}, {"str", struct_types["DT_str"]}};

  for (auto &pair : PriorityProtos) {
      pair.second->codegen();
  }


  Generate_LLVM_Functions();
  Generate_Lib_Functions();
  RegisterData();
  Generate_Class_Types();

  //===----------------------------------------------------------------------===//
  // Scalar   Operations
  //===----------------------------------------------------------------------===//

  // 
  FunctionType *fmaxTy = FunctionType::get( //TODO: automatic type detection for max and min
      Type::getFloatTy(*TheContext),
      {Type::getFloatTy(*TheContext), Type::getFloatTy(*TheContext)},
      false
  );
  TheModule->getOrInsertFunction("max", fmaxTy);


  FunctionType *mallocType = FunctionType::get(
        int8PtrTy, 
        {int64Ty}, 
        false
  );
  TheModule->getOrInsertFunction("malloc", mallocType);

  // 
  FunctionType *fminTy = FunctionType::get(
      Type::getFloatTy(*TheContext),
      {Type::getFloatTy(*TheContext), Type::getFloatTy(*TheContext)},
      false
  );
  TheModule->getOrInsertFunction("min", fminTy);


  // 
  FunctionType *flog2Ty = FunctionType::get(
      Type::getFloatTy(*TheContext),
      {Type::getFloatTy(*TheContext)},
      false
  );
  TheModule->getOrInsertFunction("logE2f", flog2Ty);


  // 
  FunctionType *roundTy = FunctionType::get(
      Type::getFloatTy(*TheContext),
      {Type::getFloatTy(*TheContext)},
      false
  );
  TheModule->getOrInsertFunction("roundE", roundTy);


  // 
  FunctionType *logical_notTy = FunctionType::get(
      Type::getFloatTy(*TheContext),
      {Type::getFloatTy(*TheContext)},
      false
  );
  TheModule->getOrInsertFunction("logical_not", logical_notTy);


  // 
  FunctionType *dir_existsTy = FunctionType::get(
      Type::getFloatTy(*TheContext),
      {int8PtrTy},
      false
  );
  TheModule->getOrInsertFunction("dir_exists", dir_existsTy);


  // 
  FunctionType *path_existsTy = FunctionType::get(
      Type::getFloatTy(*TheContext),
      {int8PtrTy},
      false
  );
  TheModule->getOrInsertFunction("path_exists", path_existsTy);


  // 
  FunctionType *floorTy = FunctionType::get(
      Type::getFloatTy(*TheContext),
      {Type::getFloatTy(*TheContext)},
      false
  );
  TheModule->getOrInsertFunction("floorE", floorTy);

  //
  FunctionType *str_DeleteTy = FunctionType::get(
      int8PtrTy,
      {int8PtrTy},
      false
  );
  TheModule->getOrInsertFunction("str_Delete", str_DeleteTy);


  //  
  FunctionType *sleepTy = FunctionType::get(
      Type::getVoidTy(*TheContext),
      {int8PtrTy, Type::getFloatTy(*TheContext)},
      false
  );
  TheModule->getOrInsertFunction("__slee_p_", sleepTy);

  
  FunctionType *silent_sleepTy = FunctionType::get(
      Type::getFloatTy(*TheContext),
      {int8PtrTy, Type::getFloatTy(*TheContext)},
      false
  );
  TheModule->getOrInsertFunction("silent_sleep", silent_sleepTy);



	FunctionType *pthread_create_auxTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("pthread_create_aux", pthread_create_auxTy);

	FunctionType *pthread_join_auxTy= FunctionType::get(
		int8PtrTy,
		{int64Ty},
		false
	);
	TheModule->getOrInsertFunction("pthread_join_aux", pthread_join_auxTy);









  

  FunctionType *LockTy = FunctionType::get(
    Type::getVoidTy(*TheContext),
    {int8PtrTy},
    false);
  TheModule->getOrInsertFunction("LockMutex", LockTy);

  FunctionType *UnlockMutexTy = FunctionType::get(
    Type::getVoidTy(*TheContext),
    {int8PtrTy},
    false);
  TheModule->getOrInsertFunction("UnlockMutex", UnlockMutexTy);


  //===----------------------------------------------------------------------===//
  // Str Ops
  //===----------------------------------------------------------------------===//






  //
  FunctionType *to_stringTy = FunctionType::get(
      int8PtrTy,
      {Type::getFloatTy(*TheContext)},
      false
  );
  TheModule->getOrInsertFunction("to_string", to_stringTy);


  //
  FunctionType *cat_str_floatTy = FunctionType::get(
      Type::getFloatTy(*TheContext),
      {int8PtrTy},
      false
  );
  TheModule->getOrInsertFunction("cat_str_float", cat_str_floatTy);


  

  FunctionType *PrintFloatTy = FunctionType::get(
      Type::getVoidTy(*TheContext),
      {Type::getFloatTy(*TheContext)},
      false 
  );
  TheModule->getOrInsertFunction("PrintFloat", PrintFloatTy);

  FunctionType *UnbugFloatTy = FunctionType::get(
      Type::getFloatTy(*TheContext),
      {Type::getFloatTy(*TheContext)},
      false 
  );
  TheModule->getOrInsertFunction("UnbugFloat", UnbugFloatTy);

 
  
  FunctionType *SplitStringTy = FunctionType::get(
      int8PtrTy,
      {int8PtrTy, int8PtrTy, int8PtrTy},
      false 
  );
  TheModule->getOrInsertFunction("SplitString", SplitStringTy);


  //
  FunctionType *SplitStringIndexateTy = FunctionType::get(
      int8PtrTy,
      {int8PtrTy, int8PtrTy, int8PtrTy, Type::getFloatTy(*TheContext)},
      false 
  );
  TheModule->getOrInsertFunction("str_split_idx", SplitStringIndexateTy);

  
  //
  FunctionType *str_to_floatTy = FunctionType::get(
      Type::getFloatTy(*TheContext),
      {int8PtrTy, int8PtrTy},
      false 
  );
  TheModule->getOrInsertFunction("str_to_float", str_to_floatTy);


  //
  FunctionType *CopyStringTy = FunctionType::get(
      int8PtrTy,
      {int8PtrTy},
      false 
  );
  TheModule->getOrInsertFunction("CopyString", CopyStringTy);


  //
  FunctionType *appendTy = FunctionType::get(
      Type::getFloatTy(*TheContext),
      {int8PtrTy, int8PtrTy},
      false 
  );
  TheModule->getOrInsertFunction("append", appendTy);


  //
  FunctionType *objAttr_var_from_varTy = FunctionType::get(
      Type::getVoidTy(*TheContext),
      {int8PtrTy, int8PtrTy},
      false 
  );
  TheModule->getOrInsertFunction("objAttr_var_from_var", objAttr_var_from_varTy);


  //
  FunctionType *objAttr_var_from_vecTy = FunctionType::get(
      Type::getVoidTy(*TheContext),
      {int8PtrTy, int8PtrTy},
      false 
  );
  TheModule->getOrInsertFunction("objAttr_var_from_vec", objAttr_var_from_vecTy);


  //
  FunctionType *objAttr_vec_from_varTy = FunctionType::get(
      Type::getVoidTy(*TheContext),
      {int8PtrTy, int8PtrTy},
      false 
  );
  TheModule->getOrInsertFunction("objAttr_vec_from_var", objAttr_vec_from_varTy);


  //
  FunctionType *objAttr_vec_from_vecTy = FunctionType::get(
      Type::getVoidTy(*TheContext),
      {int8PtrTy, int8PtrTy},
      false 
  );
  TheModule->getOrInsertFunction("objAttr_vec_from_vec", objAttr_vec_from_vecTy);





  //
  FunctionType *PrintStrTy = FunctionType::get(
      Type::getFloatTy(*TheContext),
      {int8PtrTy}, 
      false 
  );
  TheModule->getOrInsertFunction("PrintStr", PrintStrTy);


  //
  FunctionType *PrintStrVecTy = FunctionType::get(
      Type::getFloatTy(*TheContext),
      {int8PtrTy}, 
      false 
  );
  TheModule->getOrInsertFunction("PrintStrVec", PrintStrVecTy);


  //
  FunctionType *PrintFloatVecTy = FunctionType::get(
      Type::getFloatTy(*TheContext),
      {int8PtrTy}, 
      false 
  );
  TheModule->getOrInsertFunction("PrintFloatVec", PrintFloatVecTy);



  FunctionType *str_vec_printTy = FunctionType::get(
      Type::getFloatTy(*TheContext),
      {int8PtrTy, int8PtrTy}, 
      false 
  );
  TheModule->getOrInsertFunction("str_vec_print", str_vec_printTy);


  //
  FunctionType *LoadStrVecOnDemandTy = FunctionType::get(
      int8PtrTy,
      {int8PtrTy},
      false
  );
  TheModule->getOrInsertFunction("LoadStrVecOnDemand", LoadStrVecOnDemandTy);


  
  


  //
  FunctionType *LenStrVecTy = FunctionType::get(
      Type::getFloatTy(*TheContext),
      {int8PtrTy, int8PtrTy},
      false 
  );
  TheModule->getOrInsertFunction("LenStrVec", LenStrVecTy);


  //
  FunctionType *ShuffleStrVecTy = FunctionType::get(
      int8PtrTy,
      {int8PtrTy, int8PtrTy},
      false 
  );
  TheModule->getOrInsertFunction("ShuffleStrVec", ShuffleStrVecTy);


  //
  FunctionType *IndexStrVecTy = FunctionType::get(
      int8PtrTy,
      {int8PtrTy, Type::getFloatTy(*TheContext)}, 
      false 
  );
  TheModule->getOrInsertFunction("IndexStrVec", IndexStrVecTy);


  //
  FunctionType *IndexClassStrVecTy = FunctionType::get(
      int8PtrTy,
      {int8PtrTy, int8PtrTy, Type::getFloatTy(*TheContext)}, 
      false 
  );
  TheModule->getOrInsertFunction("str_vec_Idx", IndexClassStrVecTy);

  


  FunctionType *nullptr_getTy = FunctionType::get(
      int8PtrTy,
      {}, 
      false 
  );
  TheModule->getOrInsertFunction("nullptr_get", nullptr_getTy);


  // char *
  FunctionType *shuffle_strTy = FunctionType::get(
      int8PtrTy,
      {int8PtrTy},
      false 
  );
  TheModule->getOrInsertFunction("shuffle_str", shuffle_strTy);


  //===----------------------------------------------------------------------===//
  // DT_file
  //===----------------------------------------------------------------------===//

  //
  FunctionType *openTy = FunctionType::get(
      intTy,
      {int8PtrTy, intTy},
      false 
  );
  TheModule->getOrInsertFunction("open", openTy);

  //
  FunctionType *readTy = FunctionType::get(
      int64Ty,
      {intTy, int8PtrTy, intTy},
      false 
  );
  TheModule->getOrInsertFunction("read", readTy);

  //
  FunctionType *closeTy = FunctionType::get(
      voidTy,
      {intTy},
      false 
  );
  TheModule->getOrInsertFunction("close", closeTy);

  //
  FunctionType *fexistsTy = FunctionType::get(
      boolTy,
      {int8PtrTy, int8PtrTy},
      false 
  );
  TheModule->getOrInsertFunction("fexists", fexistsTy);


  

  //===----------------------------------------------------------------------===//
  // Other Ops
  //===----------------------------------------------------------------------===//
 

  //
  FunctionType *GetEmptyCharTy = FunctionType::get(
      int8PtrTy,
      {},
      false 
  );
  TheModule->getOrInsertFunction("GetEmptyChar", GetEmptyCharTy);
  

  //
  FunctionType *FreeCharTy = FunctionType::get(
      Type::getVoidTy(*TheContext),
      {int8PtrTy},
      false
  );
  TheModule->getOrInsertFunction("FreeChar", FreeCharTy);
  

  //
  FunctionType *FreeCharFromFuncTy = FunctionType::get(
      Type::getVoidTy(*TheContext),
      {int8PtrTy},
      false
  );
  TheModule->getOrInsertFunction("FreeCharFromFunc", FreeCharFromFuncTy);
  

  //
  FunctionType * ConcatStrTy = FunctionType::get(
      int8PtrTy,
      {int8PtrTy, int8PtrTy},
      false 
  );
  TheModule->getOrInsertFunction("ConcatStr", ConcatStrTy);
  

  //
  FunctionType * ConcatStrFreeLeftTy = FunctionType::get(
      int8PtrTy,
      {int8PtrTy, int8PtrTy},
      false 
  );
  TheModule->getOrInsertFunction("ConcatStrFreeLeft", ConcatStrFreeLeftTy);
  

  //
  FunctionType * ConcatStrFreeRightTy = FunctionType::get(
      int8PtrTy,
      {int8PtrTy, int8PtrTy},
      false 
  );
  TheModule->getOrInsertFunction("ConcatStrFreeRight", ConcatStrFreeRightTy);
  

  //
  FunctionType * ConcatStrFreeTy = FunctionType::get(
      int8PtrTy,
      {int8PtrTy, int8PtrTy},
      false 
  );
  TheModule->getOrInsertFunction("ConcatStrFree", ConcatStrFreeTy);

  
  //
  FunctionType * ConcatNumToStrTy = FunctionType::get(
      int8PtrTy,
      {int8PtrTy, Type::getInt32Ty(*TheContext)},
      false 
  );
  TheModule->getOrInsertFunction("ConcatNumToStr", ConcatNumToStrTy);

  
  //
  FunctionType * ConcatNumToStrFreeTy = FunctionType::get(
      int8PtrTy,
      {int8PtrTy, Type::getInt32Ty(*TheContext)},
      false 
  );
  TheModule->getOrInsertFunction("ConcatNumToStrFree", ConcatNumToStrFreeTy);
  

  //
  FunctionType * ConcatScopeStrTy = FunctionType::get(
      int8PtrTy,
      {int8PtrTy, int8PtrTy},
      false 
  );
  TheModule->getOrInsertFunction("ConcatScopeStr", ConcatScopeStrTy);
    
  
  FunctionType *print_codegenTy = FunctionType::get(
      int8PtrTy,
      {int8PtrTy},
      false 
  );
  TheModule->getOrInsertFunction("print_codegen", print_codegenTy);

  // FunctionType *GC_write_barrier_objTy= FunctionType::get(
		// Type::getVoidTy(*TheContext),
		// {int8PtrTy, int8PtrTy, int8PtrTy->getPointerTo(), int8PtrTy, Type::getInt16Ty(*TheContext)},
		// false
  // );
  // TheModule->getOrInsertFunction("GC_write_barrier_obj", GC_write_barrier_objTy);

  TheModule->getOrInsertFunction(
    "posix_memalign",
    FunctionType::get(Type::getInt32Ty(*TheContext),
                      {int8PtrTy->getPointerTo(), // void**
                       Type::getInt64Ty(*TheContext),                   // alignment
                       Type::getInt64Ty(*TheContext)},                  // size
                      false));

  //
  FunctionType *printTy = FunctionType::get(
      voidTy,
      {int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy}, 
      true 
  );
  TheModule->getOrInsertFunction("print", printTy);

  FunctionType *printfTy = FunctionType::get(
      intTy,
      int8PtrTy, 
      true 
  );
  TheModule->getOrInsertFunction("printf", printfTy);
  

}



ThreadSafeModule irgenAndTakeOwnership(FunctionAST &FnAST,
                                       const std::string &Suffix) {
  if (auto *F = FnAST.codegen()) {
    F->setName(F->getName() + Suffix);
    auto TSM = ThreadSafeModule(std::move(TheModule), std::move(TheContext));
    // Start a new module.
    InitializeModule();
    return TSM;
  } else
    report_fatal_error("failed to JIT.");
}

// nlohmann::json FunctionAST::toJSON() {
//     std::cout << "FUNCTION AST JSON" << ".\n";
    
//     nlohmann::json j;
//     j["type"] = "function";
//     j["name"] = Proto->getName();
//     j["return"] = Proto->ReturnType.Type;

//     j["args"] = nlohmann::json::array();
//     int len = Proto->Args.size();
//     for (int i=1; i<len; ++i) {
//         nlohmann::json arg_j;
//         arg_j["name"] = Proto->Args[i];
//         arg_j["type"] = Proto->Types[i-1].toString();
//         j["args"].push_back(arg_j);
//     }
//     return j;
// }


const PrototypeAST& FunctionAST::getProto() const {
  return *Proto;
}

const std::string& FunctionAST::getName() const {
  return Proto->getName();
}


void SetKernelVars(std::string function_name) {
    FunctionCallee tid_x =
    PtxModule->getOrInsertFunction(
        "llvm.nvvm.read.ptx.sreg.tid.x",
        Builder->getInt32Ty());

    FunctionCallee ctaid_x =
        PtxModule->getOrInsertFunction(
            "llvm.nvvm.read.ptx.sreg.ctaid.x",
            Builder->getInt32Ty());

    FunctionCallee ntid_x =
        PtxModule->getOrInsertFunction(
            "llvm.nvvm.read.ptx.sreg.ntid.x",
            Builder->getInt32Ty());

    FunctionCallee nctaid_x =
        PtxModule->getOrInsertFunction(
            "llvm.nvvm.read.ptx.sreg.nctaid.x",
            Builder->getInt32Ty());

    Value *tid  = Builder->CreateCall(tid_x);
    Value *bid  = Builder->CreateCall(ctaid_x);
    Value *bdim = Builder->CreateCall(ntid_x);

    function_values[function_name]["tx"] = tid;
    function_values[function_name]["bx"] = bid;
}


Function *FunctionAST::codegen_gpu(int idx) {

  auto &P = *Proto;

  // FunctionProtos[Proto->getName()] = std::move(Proto);
  std::string function_name = P.getName();


  std::string fn_name = function_name;
  if (idx>=0) {
      fn_name += std::to_string(idx);
      parser_struct.cvalues = Fn_Compiled_Values[fn_name];
  }

  Function *TheFunction = PtxModule->getFunction(fn_name); 
  if(!TheFunction)
    std::cout << "PTX function " << function_name << " not found.\n";



  auto oldBuilder = std::move(Builder);
  CurModule = PtxModule.get();

  Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);

  BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
  Builder->SetInsertPoint(BB);


  Value *scope_struct = ConstantPointerNull::get(
            cast<PointerType>(int8PtrTy)
        );

  int args_count = P.Args.size();
  auto it = TheFunction->arg_begin();
  for (int i=0; i<args_count; ++i, ++it) {
        llvm::Argument &Arg = *it;

        std::string arg_name = Arg.getName().str();
        function_values[function_name][arg_name] = &Arg;
  }

  SetKernelVars(function_name);

  for (auto &body : Body) {
    if (idx>=0) {// is comp specialization
        std::cout << "set cvalues " << parser_struct.cvalues.ints.size() << "\n";
        body->SetCValues(parser_struct);
    }
    body->codegen(scope_struct);
  }
  
  if(!Builder->GetInsertBlock()->getTerminator())
      Builder->CreateRetVoid(); 


  Builder = std::move(oldBuilder);
  CurModule = TheModule.get();


  if (verifyFunction(*TheFunction, &errs()))
    errs() << "Invalid function!\n";


  return TheFunction;
}












Function *FunctionAST::codegen() {
  if (not ShallCodegen)
    return nullptr;


  if (parser_struct.gpu>0)
      return codegen_gpu();
  
  // Transfer ownership of the prototype to the FunctionProtos map, but keep a
  // reference to it for use below.
  auto &P = *Proto;

    

  FunctionProtos[Proto->getName()] = std::move(Proto);
  std::string function_name = P.getName();


  Function *TheFunction = getFunction(function_name);
  if (!TheFunction)
    return nullptr;

  // If this is an operator, install it.
  if (P.isBinaryOp())
    BinopPrecedence[P.getOperatorName()] = P.getBinaryPrecedence();

  // Create a new basic block to start insertion into.
  BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
  Builder->SetInsertPoint(BB);


  current_codegen_function = function_name;
  //   LogBlue("Execute function: " + function_name);
  

  

  Value *scope_struct;
  if(function_name=="__anon_expr") {
    scope_struct = callret("scope_struct_CreateFirst", {}); 
    call("scope_struct_Alloc_GC", {scope_struct});
    function_values[current_codegen_function]["QQ_stack_top"] = const_int(0);
    fn_stack_offset[current_codegen_function] = 0;
  }
  
  



  float val;
  int i = 0;
  int args_count = P.Args.size();
  auto it = TheFunction->arg_begin();
  Parser_Struct parser_struct;
  parser_struct.function_name = current_codegen_function;
  for (int i=0; i<args_count; ++i, ++it) {
    llvm::Argument &Arg = *it;

    std::string arg_name = Arg.getName().str();

    if (arg_name == "scope_struct") {
        StructType *st = struct_types["scope_struct"];
        scope_struct = &Arg;
        Value *stack_top_value_gep = Builder->CreateStructGEP(st, scope_struct, 3); 
        function_values[current_codegen_function]["QQ_stack_top"] = Builder->CreateLoad(intTy, stack_top_value_gep);
        fn_stack_offset[current_codegen_function] = 0;
    } else {
        function_values[current_codegen_function][arg_name] = &Arg;
        StoreVal(TheFunction, current_codegen_function, arg_name, &Arg,
                    data_typeVars[current_codegen_function][arg_name]);
    }
   
  }
  
  Value *RetVal;

  if(function_name=="main") {
    scope_struct = callret("scope_struct_CreateFirst", {}); 
    call("prebuild", {});
    call("scope_struct_Alloc_GC", {scope_struct});
    function_values[current_codegen_function]["QQ_stack_top"] = const_int(0);
    fn_stack_offset[current_codegen_function] = 0;
  }

  for (auto &body : Body)
    RetVal = body->codegen(scope_struct);
  cur_self = nullptr;

  if (RetVal) {
    // Finish off the function.
    if(!Builder->GetInsertBlock()->getTerminator()) {
        Builder->CreateRet(RetVal); 
    }

    // Validate the generated code, checking for consistency.
    verifyFunction(*TheFunction);
    // TheModule->print(llvm::errs(), nullptr);

    return TheFunction;
  } 

  // Error reading body, remove function.
  TheFunction->eraseFromParent();

  if (P.isBinaryOp())
    BinopPrecedence.erase(P.getOperatorName());
  return nullptr;
}

