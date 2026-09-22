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
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Target/TargetMachine.h"


#include "KaleidoscopeJIT.h"

#include <algorithm>
#include <cstdarg>
#include <cassert>
#include <cctype>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <iostream>
#include <numeric>
#include <utility>
#include <vector>
#include <iomanip>
#include <math.h>
#include <fenv.h>
#include <tuple>
#include <chrono>
#include <thread>
#include <random>
#include <float.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "compiler_frontend/escape_analysis.h"
#include "compiler_frontend/modules.h"
#include "compiler_frontend/ownership.h"
#include "include.h"
#include "runtime/compiler_frontend/global_vars.h"


using namespace llvm;
using namespace llvm::orc;




LCG rng(generate_custom_seed());


  // Error Colors
// \033[0m default
// \033[31m red
// \033[33m yellow
// \033[34m blue
// \033[95m purple


//===----------------------------------------------------------------------===//
// Code Generation
//===----------------------------------------------------------------------===//

//global


std::vector<std::unique_ptr<FunctionAST>> AllFunctions;
std::unordered_map<std::string,std::unordered_map<CallArgsTy,FunctionAST*,ArgsHasher,ArgsEqual>> Template_FnAST;
std::unordered_map<std::string, std::unique_ptr<FunctionAST>> GpuFunctions;

// Vars
std::map<std::string, std::vector<char *>> ClassStrVecs;
std::map<std::string, float> NamedClassValues;
std::map<std::string, std::vector<std::pair<std::string, std::string>>> ScopeVarsToClean;
std::map<std::string, char *> ScopeNamesToClean;
std::map<int, std::map<std::string, std::vector<std::string>>> ThreadedScopeTensorsToClean;






// File Handling
std::vector<char *> glob_str_files;



void register_version(std::string fn) {
    std::vector<Data_Tree> dts;
    for (auto &arg : Function_Arg_DataTypes[fn]) {
        Data_Tree dt = arg.second;
        dts.push_back(dt);
    }
    SetFnVersion(fn, dts);
}

void register_call_args_ty() {
    for (auto &fn : fn_ret_dt)
        register_version(fn.first);
    for (auto &fn : function_return_overwrite)
        register_version(fn.first);
    for (auto &fn : method_return_overwrite) 
        register_version(fn.first);
}


//===----------------------------------------------------------------------===//
// Top-Level parsing and JIT Driver
//===----------------------------------------------------------------------===//

void HandleExtern() {
  Parser_Struct *parser_struct = new Parser_Struct();
  if (auto ProtoAST = ParseExtern(parser_struct)) {
    if (auto *FnIR = ProtoAST->codegen()) {
      fprintf(stderr, "Read extern: ");
      FnIR->print(errs());
      fprintf(stderr, "\n");
      FunctionProtos[ProtoAST->getName()] = std::move(ProtoAST);
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

void HandleProto() {
  Parser_Struct *parser_struct = new Parser_Struct();
  ParseProtoExpr(parser_struct, "");
}
void HandleOp() {
  Parser_Struct *parser_struct = new Parser_Struct();
  ParseOpExpr(parser_struct, "");
}

void HandleDefinition() {
  
  Parser_Struct *parser_struct = new Parser_Struct();
  if (auto FnAST = ParseDefinition(parser_struct)) {
    if (FnAST->getProto().is_generic) {
        TheJIT->addGeneric(std::move(FnAST));
        return;
    }

    FunctionProtos[FnAST->getProto().getName()] =
      std::make_unique<PrototypeAST>(FnAST->getProto());

    if (IsJIT)
        ExitOnErr(TheJIT->addAST(std::move(FnAST)));
    else
        AllFunctions.push_back(std::move(FnAST));

  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}


void HandleGpuDef() {
  Parser_Struct *parser_struct = new Parser_Struct();
  parser_struct->gpu=(CurTok==tok_kernel) ? 1 : 2;

  if (auto FnAST = ParseDefinition(parser_struct)) {
    std::string fn_name = FnAST->getProto().getName();
    if (FnAST->getProto().is_generic) {
        TheJIT->addGeneric(std::move(FnAST));
        return;
    }

    FunctionProtos[fn_name] =
      std::make_unique<PrototypeAST>(FnAST->getProto());
    GpuFunctions[fn_name] = std::move(FnAST);

    bool has_compiled_args = Fn_Compiled_Args.count(fn_name)>0;
    bool is_template = FnTemplates.count(fn_name)>0;

    if (!has_compiled_args&&!is_template) {
        getGpuFnCheck(fn_name);
    }

  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

void HandleImport() {
    Parser_Struct *parser_struct = new Parser_Struct();
    parser_struct->line = tokenizer->Line;
    ParseImport(parser_struct);
}

void HandleClass() {
    Parser_Struct *parser_struct = new Parser_Struct();
    parser_struct->line = tokenizer->Line;
    ParseClass(parser_struct);
}


void CodegenTopLevelExpression(std::unique_ptr<FunctionAST> &FnAST) {

    auto Err = TheJIT->addAST(std::move(FnAST));

    // std::cout << "MAIN" << "\n";
    for(auto & fn : prebuild_functions)
        FunctionChecks(fn);
    FunctionChecks("__anon_expr");
    std::unordered_map<std::string, int> seen_escapes, seen_borrows;
    EscapeAnalysis("__anon_expr", "__anon_expr", seen_escapes);
    BorrowChecker("__anon_expr", "__anon_expr", seen_borrows);

    TheJIT->genAST();
    // TheModule->print(llvm::errs(), nullptr);
   
    // if (verifyModule(*TheModule, &errs())) {
    //     errs() << "Module invalid\n";
    //     abort();
    // }

    PtxModule.reset();
    auto TSM = llvm::orc::ThreadSafeModule(
        std::move(TheModule),
        std::move(TheContext)
    );

    Err = TheJIT->JIT->addIRModule(std::move(TSM));
    if (Err)
        ExitOnErr(std::move(Err));




    auto Sym = TheJIT->JIT->lookup("__anon_expr");
    auto *FP = Sym->toPtr<float (*)()>();
    float Result = FP();

    // InitializeModule();
}



void HandleTopLevelExpression() {
  
  Parser_Struct *parser_struct = new Parser_Struct();
  parser_struct->function_name = "__anon_expr";

  if (std::unique_ptr<FunctionAST> FnAST = ParseTopLevelExpr(parser_struct)) {
    CodegenTopLevelExpression(std::ref(FnAST));	
  
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

/// top ::= definition | external | expression | ';'
void MainLoop() {
    while (true) {
         // std::cout << "MAIN LOOP, reading token: " << CurTok << "/" << ReverseToken(CurTok) << "\n";
        switch (CurTok) {
        case 13:
            std::cout << "FOUND CARRIAGE RETURN" << ".\n";
            break;
        case tok_eof:
            if (tokenizer->inner) {
                tokenizer = std::move(tokenizer->inner);
                CurTok = tokenizer->LastToken;
                break;
            }
            return;
        case ';': // ignore top-level semicolons.
            getNextToken();
            break;
        case '.': 
            getNextToken();
            break;
        case tok_space:
            getNextToken();
            break;
        case tok_tab:
            getNextToken();
            break;
        case tok_def:
            HandleDefinition();
            break;
        case tok_gpu:
            HandleGpuDef();
            break;
        case tok_kernel:
            HandleGpuDef();
            break;
        case tok_op:
            HandleOp();
            break;
        case tok_proto:
            HandleProto();
            break;
        case tok_main:
            Generate_Class_Types();
            if(IsJIT)
                HandleTopLevelExpression(); 
            else
                HandleDefinition();
            break;
        case tok_class:
            HandleClass();
            break;
        case tok_import:
            HandleImport();
            break;
        case tok_extern:
            HandleExtern();
            break;
        case tok_constructor:
            LogErrorNextBlock(tokenizer->Line, "Constructor has no class associated.");
            break;
        default:
            // std::cout << "Wait top level" <<  ".\n";
            // std::cout << "reading token: " << CurTok << "/" << ReverseToken(CurTok) << "\n";
            HandleTopLevelExpression(); 
            // std::cout << "Finished top level" <<  ".\n";
            break;
        }
    }
}


void InitializeTokenizer() {
    std::string lib_path = std::getenv("NEVE_LIBS");
    std::string std_path = lib_path + "/std_lib/include.nv";

    ParseClasses(std_path);
    if (Sys_Arguments.size()>0)
        ParseClasses(Sys_Arguments[0]);

    tokenizer = std::make_unique<Tokenizer>(std_path);
    getNextToken();
    MainLoop();

    if (Sys_Arguments.size()>0)
        tokenizer = std::make_unique<Tokenizer>(Sys_Arguments[0]);
    else
        tokenizer = std::make_unique<Tokenizer>(""); //todo: save stdin tokenizer
    getNextToken();
}


//===----------------------------------------------------------------------===//
// "Library" functions that can be "extern'd" from user code.
//===----------------------------------------------------------------------===//

/// putchard - putchar that takes a float and returns 0.
extern "C" float putchard(float X) {
  fputc((char)X, stderr);
  return 0;
}

/// printd - printf that takes a float prints it as "%f\n", returning 0.
extern "C" float printd(float X) {
  fprintf(stderr, "%f\n", X);
  return 0;
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//


void build_dicts() {
  // DT_charv
  Function_Arg_DataTypes["charv_Create"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["charv_Create"]["1"] = Data_Tree("int");
  fn_argnames["charv_Create"] = {"0", "1"};

  // DT_vec
  Function_Arg_DataTypes["vec_Create"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["vec_Create"]["1"] = Data_Tree("int");
  Function_Arg_DataTypes["vec_Create"]["2"] = Data_Tree("int");
  fn_argnames["vec_Create"] = {"0", "1", "2"};



  // min
  function_return_overwrite["min"] = min_ret_dt;
  Function_Arg_DataTypes["min"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["min"]["1"] = Data_Tree("any");
  Function_Arg_DataTypes["min"]["2"] = Data_Tree("any");
  fn_argnames["min"] = {"0", "1", "2"};
  Function_Required_Arg_Count["min"] = 2;

  // max
  function_return_overwrite["max"] = max_ret_dt;
  Function_Arg_DataTypes["max"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["max"]["1"] = Data_Tree("any");
  Function_Arg_DataTypes["max"]["2"] = Data_Tree("any");
  fn_argnames["max"] = {"0", "1", "2"};
  Function_Required_Arg_Count["max"] = 2;



  // c_open
  fn_ret_dt["c_open"] = Data_Tree("int");
  Function_Arg_DataTypes["c_open"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["c_open"]["1"] = Data_Tree("str");
  fn_argnames["c_open"] = {"0", "1"};
  Function_Required_Arg_Count["c_open"] = 1;

  // c_read
  fn_ret_dt["c_read"] = Data_Tree("i64");
  Function_Arg_DataTypes["c_read"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["c_read"]["1"] = Data_Tree("int");
  Function_Arg_DataTypes["c_read"]["2"] = Data_Tree("charv");
  Function_Arg_DataTypes["c_read"]["3"] = Data_Tree("int");
  fn_argnames["c_read"] = {"0", "1", "2", "3"};
  Function_Required_Arg_Count["c_read"] = 3;



  // c_strlen
  fn_ret_dt["c_strlen"] = Data_Tree("i64");
  Function_Arg_DataTypes["c_strlen"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["c_strlen"]["1"] = Data_Tree("str");
  fn_argnames["c_strlen"] = {"0", "1"};
  Function_Required_Arg_Count["c_strlen"] = 1;



  // str_set
  fn_ret_dt["str_set"] = Data_Tree("int");
  Function_Arg_DataTypes["str_set"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["str_set"]["1"] = Data_Tree("str");
  Function_Arg_DataTypes["str_set"]["2"] = Data_Tree("int");
  Function_Arg_DataTypes["str_set"]["3"] = Data_Tree("int");
  fn_argnames["str_set"] = {"0", "1", "2", "3"};
  Function_Required_Arg_Count["str_set"] = 3;

  // str_offset
  fn_ret_dt["str_offset"] = Data_Tree("str");
  Function_Arg_DataTypes["str_offset"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["str_offset"]["1"] = Data_Tree("str");
  Function_Arg_DataTypes["str_offset"]["2"] = Data_Tree("int");
  fn_argnames["str_offset"] = {"0", "1", "2"};
  Function_Required_Arg_Count["str_offset"] = 2;

  // err
  fn_ret_dt["err"] = Data_Tree("int");
  Function_Arg_DataTypes["err"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["err"]["1"] = Data_Tree("str");
  fn_argnames["err"] = {"0", "1"};
  Function_Required_Arg_Count["err"] = 1;


  
  // to_char
  fn_ret_dt["to_char"] = Data_Tree("char");
  Function_Arg_DataTypes["to_char"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["to_char"]["1"] = Data_Tree("any");
  fn_argnames["to_char"] = {"0", "1"};
  Function_Required_Arg_Count["to_char"] = 1;
  // i8
  fn_ret_dt["i8"] = Data_Tree("i8");
  Function_Arg_DataTypes["i8"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["i8"]["1"] = Data_Tree("any");
  fn_argnames["i8"] = {"0", "1"};
  Function_Required_Arg_Count["i8"] = 1;
  // i16
  fn_ret_dt["i16"] = Data_Tree("i16");
  Function_Arg_DataTypes["i16"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["i16"]["1"] = Data_Tree("any");
  fn_argnames["i16"] = {"0", "1"};
  Function_Required_Arg_Count["i16"] = 1;
  // int
  fn_ret_dt["to_int"] = Data_Tree("int");
  Function_Arg_DataTypes["to_int"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["to_int"]["1"] = Data_Tree("any");
  fn_argnames["to_int"] = {"0", "1"};
  Function_Required_Arg_Count["to_int"] = 1;
  // i64
  fn_ret_dt["i64"] = Data_Tree("i64");
  Function_Arg_DataTypes["i64"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["i64"]["1"] = Data_Tree("any");
  fn_argnames["i64"] = {"0", "1"};
  Function_Required_Arg_Count["i64"] = 1;
  // bf16
  fn_ret_dt["bf16"] = Data_Tree("bf16");
  Function_Arg_DataTypes["bf16"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["bf16"]["1"] = Data_Tree("any");
  fn_argnames["bf16"] = {"0", "1"};
  Function_Required_Arg_Count["bf16"] = 1;
  // to_float
  fn_ret_dt["to_float"] = Data_Tree("float");
  Function_Arg_DataTypes["to_float"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["to_float"]["1"] = Data_Tree("any");
  fn_argnames["to_float"] = {"0", "1"};
  Function_Required_Arg_Count["to_float"] = 1;

  // ctz
  fn_ret_dt["ctz"] = Data_Tree("int");
  Function_Arg_DataTypes["ctz"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["ctz"]["1"] = Data_Tree("any");
  fn_argnames["ctz"] = {"0", "1"};
  Function_Required_Arg_Count["ctz"] = 1;

  // swap_bit
  function_return_overwrite["swap_bit"] = swap_bit_ret;
  Function_Arg_DataTypes["swap_bit"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["swap_bit"]["1"] = Data_Tree("any");
  Function_Arg_DataTypes["swap_bit"]["2"] = Data_Tree("int");
  fn_argnames["swap_bit"] = {"0", "1", "2"};
  Function_Required_Arg_Count["swap_bit"] = 2;

  fn_ret_dt["printff"] = Data_Tree("int");
  Function_Arg_DataTypes["printff"]["0"] = Data_Tree("any");
  Function_Arg_DataTypes["printff"]["1"] = Data_Tree("any");
  fn_argnames["printff"] = {"0", "1"};
  Function_Required_Arg_Count["printff"] = 2;


  // shfl_sync
  function_return_overwrite["shfl_sync"] = shfl_sync_ret;
  Function_Arg_DataTypes["shfl_sync"]["0"] = Data_Tree("any");
  Function_Arg_DataTypes["shfl_sync"]["1"] = Data_Tree("int");
  fn_argnames["shfl_sync"] = {"0", "1"};
  Function_Required_Arg_Count["shfl_sync"] = 2;

  // cp_async16
  fn_ret_dt["cp_async16"] = Data_Tree("void");
  Function_Arg_DataTypes["cp_async16"]["0"] = Data_Tree("any");
  Function_Arg_DataTypes["cp_async16"]["1"] = Data_Tree("any");
  fn_argnames["cp_async16"] = {"0", "1"};
  Function_Required_Arg_Count["cp_async16"] = 2;
  // cp_commit_group
  fn_ret_dt["cp_commit_group"] = Data_Tree("void");
  Function_Required_Arg_Count["cp_commit_group"] = 0;
  // cp_wait_group
  fn_ret_dt["cp_wait_group"] = Data_Tree("int");
  Function_Arg_DataTypes["cp_wait_group"]["0"] = Data_Tree("int");
  Function_Required_Arg_Count["cp_wait_group"] = 1;
  fn_argnames["cp_wait_group"] = {"0"};
  // cp_wait_all
  fn_ret_dt["cp_wait_all"] = Data_Tree("void");
  Function_Required_Arg_Count["cp_wait_all"] = 0;
  // ldmatrix_x4
  // Data_Tree ldmatrix_x4_dt = Data_Tree("int");
  // ldmatrix_x4_dt.Nested_Data.push_back(Data_Tree("4"));
  // ldmatrix_x4_dt.is_array = true;
  fn_ret_dt["ldmatrix_x4"] = Data_Tree("any");
  Function_Arg_DataTypes["ldmatrix_x4"]["0"] = Data_Tree("any");
  Function_Arg_DataTypes["ldmatrix_x4"]["1"] = Data_Tree("any");
  Function_Required_Arg_Count["ldmatrix_x4"] = 2;
  fn_argnames["ldmatrix_x4"] = {"0", "1"};
  // ldmatrix_x2
  // Data_Tree ldmatrix_x2_dt = Data_Tree("int");
  // ldmatrix_x2_dt.Nested_Data.push_back(Data_Tree("2"));
  // ldmatrix_x2_dt.is_array = true;
  fn_ret_dt["ldmatrix_x2"] = Data_Tree("any");
  Function_Arg_DataTypes["ldmatrix_x2"]["0"] = Data_Tree("any");
  Function_Arg_DataTypes["ldmatrix_x2"]["1"] = Data_Tree("any");
  Function_Required_Arg_Count["ldmatrix_x2"] = 2;
  fn_argnames["ldmatrix_x2"] = {"0", "1"};

  fn_ret_dt["ldmatrix_x2T"] = Data_Tree("any");
  Function_Arg_DataTypes["ldmatrix_x2T"]["0"] = Data_Tree("any");
  Function_Arg_DataTypes["ldmatrix_x2T"]["1"] = Data_Tree("any");
  Function_Required_Arg_Count["ldmatrix_x2T"] = 2;
  fn_argnames["ldmatrix_x2T"] = {"0", "1"};

  // mma_16x8x16
  // Data_Tree mma_16x8x16_dt = Data_Tree("int");
  // mma_16x8x16_dt.Nested_Data.push_back(Data_Tree("4"));
  // mma_16x8x16_dt.is_array = true;
  fn_ret_dt["mma_16x8x16"] = Data_Tree("any");
  Function_Arg_DataTypes["mma_16x8x16"]["0"] = Data_Tree("any");
  Function_Arg_DataTypes["mma_16x8x16"]["1"] = Data_Tree("any");
  Function_Arg_DataTypes["mma_16x8x16"]["2"] = Data_Tree("any");
  Function_Required_Arg_Count["mma_16x8x16"] = 3;
  fn_argnames["mma_16x8x16"] = {"0", "1", "2"};
  // syncthreads
  fn_ret_dt["syncthreads"] = Data_Tree("void");
  Function_Required_Arg_Count["syncthreads"] = 0;

  // // simd_load
  function_return_overwrite["simd_load"] = simd_load_ret;

  // vec_shuffle
  function_return_overwrite["vec_shuffle"] = vec_shuffle_ret;

  // vec_make
  function_return_overwrite["vec_make"] = vec_make_ret;
  Function_Arg_DataTypes["vec_make"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["vec_make"]["1"] = Data_Tree("any");
  Function_Arg_DataTypes["vec_make"]["2"] = Data_Tree("int");
  fn_argnames["vec_make"] = {"0", "1", "2"};
  Function_Required_Arg_Count["vec_make"] = 2;

  // vec_movemask
  fn_ret_dt["vec_movemask"] = Data_Tree("int");
  Function_Arg_DataTypes["vec_movemask"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["vec_movemask"]["1"] = Data_Tree("any");
  fn_argnames["vec_movemask"] = {"0", "1"};
  Function_Required_Arg_Count["vec_movemask"] = 1;


  // vec_print
  fn_ret_dt["vec_print"] = Data_Tree("int");
  Function_Arg_DataTypes["vec_print"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["vec_print"]["1"] = Data_Tree("vec");
  fn_argnames["vec_print"] = {"0", "1"};
  Function_Required_Arg_Count["vec_print"] = 1;


  // array_copy
  method_return_overwrite["array_clone"] = array_clone_dt;
  // array_pop
  method_return_overwrite["array_pop"] = array_pop_dt;

  // map_keys
  method_return_overwrite["map_keys"] = map_keys_dt;
  // map_values
  method_return_overwrite["map_values"] = map_values_dt;
  // map_get
  method_return_overwrite["map_get_i64_any"] = map_get_dt;
  method_return_overwrite["map_get_i64_int"] = map_get_dt;
  method_return_overwrite["map_get_str_int"] = map_get_dt;



  // printl
  fn_ret_dt["printl"] = Data_Tree("void");
  Function_Arg_DataTypes["printl"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["printl"]["1"] = Data_Tree("str");
  Function_Arg_DataTypes["printl"]["2"] = Data_Tree("str");
  Function_Arg_DataTypes["printl"]["3"] = Data_Tree("str");
  Function_Arg_DataTypes["printl"]["4"] = Data_Tree("str");
  Function_Arg_DataTypes["printl"]["5"] = Data_Tree("str");
  Function_Arg_DataTypes["printl"]["6"] = Data_Tree("str");
  Function_Arg_DataTypes["printl"]["7"] = Data_Tree("str");
  Function_Arg_DataTypes["printl"]["8"] = Data_Tree("str");
  Function_Arg_DataTypes["printl"]["9"] = Data_Tree("str");
  Function_Arg_DataTypes["printl"]["10"] = Data_Tree("str");
  fn_argnames["printl"] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"};
  // print
  fn_ret_dt["print"] = Data_Tree("void");
  Function_Arg_DataTypes["print"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["print"]["1"] = Data_Tree("str");
  Function_Arg_DataTypes["print"]["2"] = Data_Tree("str");
  Function_Arg_DataTypes["print"]["3"] = Data_Tree("str");
  Function_Arg_DataTypes["print"]["4"] = Data_Tree("str");
  Function_Arg_DataTypes["print"]["5"] = Data_Tree("str");
  Function_Arg_DataTypes["print"]["6"] = Data_Tree("str");
  Function_Arg_DataTypes["print"]["7"] = Data_Tree("str");
  Function_Arg_DataTypes["print"]["8"] = Data_Tree("str");
  Function_Arg_DataTypes["print"]["9"] = Data_Tree("str");
  Function_Arg_DataTypes["print"]["10"] = Data_Tree("str");
  fn_argnames["print"] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"};

  set_functions_return_type();
  set_functions_args_type();
  set_user_functions();
  register_call_args_ty();

  // Prime the first token.
  prebuild();
}

