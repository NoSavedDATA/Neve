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

#include "include.h"


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

// Vars
std::map<std::string, std::vector<char *>> ClassStrVecs;
std::map<std::string, float> NamedClassValues;
std::map<std::string, int> NamedInts;
std::map<std::string, std::vector<std::pair<std::string, std::string>>> ScopeVarsToClean;
std::map<std::string, char *> ScopeNamesToClean;
std::map<int, std::map<std::string, std::vector<std::string>>> ThreadedScopeTensorsToClean;






// File Handling
std::vector<char *> glob_str_files;




//===----------------------------------------------------------------------===//
// Top-Level parsing and JIT Driver
//===----------------------------------------------------------------------===//

void HandleExtern() {
  Parser_Struct parser_struct;
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
  Parser_Struct parser_struct;
  ParseProtoExpr(parser_struct, "");
}
void HandleOp() {
  Parser_Struct parser_struct;
  ParseOpExpr(parser_struct, "");
}

void HandleDefinition() {
  
  Parser_Struct parser_struct;
  if (auto FnAST = ParseDefinition(parser_struct)) {

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

void HandleImport() {
    Parser_Struct parser_struct;
    parser_struct.line = tokenizer->Line;
    ParseImport(parser_struct);
}

void HandleClass() {
    Parser_Struct parser_struct;
    parser_struct.line = tokenizer->Line;
    ParseClass(parser_struct);
}


void CodegenTopLevelExpression(std::unique_ptr<FunctionAST> &FnAST) {


    
    // JIT Version
    auto *FnIR =  FnAST->codegen();
    // Create a ResourceTracker for memory managment
    // anonymous expression -- that way we can free it after executing.
    auto RT = TheJIT->getMainJITDylib().createResourceTracker();
    auto TSM = ThreadSafeModule(std::move(TheModule), std::move(TheContext));
    ExitOnErr(TheJIT->addModule(std::move(TSM), RT));
    // Add IR module

    InitializeModule();
    // Points __anon_expr
    auto Sym = ExitOnErr(TheJIT->lookup("__anon_expr"));
    // Get the symbol's address and cast it to the right type (takes no
    // arguments, returns a float) so we can call it as a native function.
    auto *FP = Sym.getAddress().toPtr<float (*)()>();
    auto fp = FP();
    // fprintf(stderr, "%.2f\n", fp);
    // Delete the anonymous expression module from the JIT.
    ExitOnErr(RT->remove());    
}



void HandleTopLevelExpression() {
  // Evaluate a top-level expression into an anonymous function.
  
  Parser_Struct parser_struct;
  parser_struct.function_name = "__anon_expr";

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
  Function_Arg_Names["charv_Create"] = {"0", "1"};

  // DT_vec
  Function_Arg_DataTypes["vec_Create"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["vec_Create"]["1"] = Data_Tree("int");
  Function_Arg_DataTypes["vec_Create"]["2"] = Data_Tree("int");
  Function_Arg_Names["vec_Create"] = {"0", "1", "2"};



  // min
  function_return_overwrite["min"] = min_ret_dt;
  Function_Arg_DataTypes["min"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["min"]["1"] = Data_Tree("any");
  Function_Arg_DataTypes["min"]["2"] = Data_Tree("any");
  Function_Arg_Names["min"] = {"0", "1", "2"};
  Function_Required_Arg_Count["min"] = 2;

  // max
  function_return_overwrite["max"] = max_ret_dt;
  Function_Arg_DataTypes["max"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["max"]["1"] = Data_Tree("any");
  Function_Arg_DataTypes["max"]["2"] = Data_Tree("any");
  Function_Arg_Names["max"] = {"0", "1", "2"};
  Function_Required_Arg_Count["max"] = 2;



  // c_open
  functions_return_data_type["c_open"] = Data_Tree("int");
  Function_Arg_DataTypes["c_open"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["c_open"]["1"] = Data_Tree("str");
  Function_Arg_Names["c_open"] = {"0", "1"};
  Function_Required_Arg_Count["c_open"] = 1;

  // c_read
  functions_return_data_type["c_read"] = Data_Tree("i64");
  Function_Arg_DataTypes["c_read"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["c_read"]["1"] = Data_Tree("int");
  Function_Arg_DataTypes["c_read"]["2"] = Data_Tree("charv");
  Function_Arg_DataTypes["c_read"]["3"] = Data_Tree("int");
  Function_Arg_Names["c_read"] = {"0", "1", "2", "3"};
  Function_Required_Arg_Count["c_read"] = 3;



  // c_strlen
  functions_return_data_type["c_strlen"] = Data_Tree("i64");
  Function_Arg_DataTypes["c_strlen"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["c_strlen"]["1"] = Data_Tree("str");
  Function_Arg_Names["c_strlen"] = {"0", "1"};
  Function_Required_Arg_Count["c_strlen"] = 1;



  // str_set
  functions_return_data_type["str_set"] = Data_Tree("int");
  Function_Arg_DataTypes["str_set"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["str_set"]["1"] = Data_Tree("str");
  Function_Arg_DataTypes["str_set"]["2"] = Data_Tree("int");
  Function_Arg_DataTypes["str_set"]["3"] = Data_Tree("int");
  Function_Arg_Names["str_set"] = {"0", "1", "2", "3"};
  Function_Required_Arg_Count["str_set"] = 3;

  // str_offset
  functions_return_data_type["str_offset"] = Data_Tree("str");
  Function_Arg_DataTypes["str_offset"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["str_offset"]["1"] = Data_Tree("str");
  Function_Arg_DataTypes["str_offset"]["2"] = Data_Tree("int");
  Function_Arg_Names["str_offset"] = {"0", "1", "2"};
  Function_Required_Arg_Count["str_offset"] = 2;

  // err
  functions_return_data_type["err"] = Data_Tree("int");
  Function_Arg_DataTypes["err"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["err"]["1"] = Data_Tree("str");
  Function_Arg_Names["err"] = {"0", "1"};
  Function_Required_Arg_Count["err"] = 1;


  
  // to_char
  functions_return_data_type["to_char"] = Data_Tree("char");
  Function_Arg_DataTypes["to_char"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["to_char"]["1"] = Data_Tree("any");
  Function_Arg_Names["to_char"] = {"0", "1"};
  Function_Required_Arg_Count["to_char"] = 1;
  // i8
  functions_return_data_type["i8"] = Data_Tree("i8");
  Function_Arg_DataTypes["i8"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["i8"]["1"] = Data_Tree("any");
  Function_Arg_Names["i8"] = {"0", "1"};
  Function_Required_Arg_Count["i8"] = 1;
  // i16
  functions_return_data_type["i16"] = Data_Tree("i16");
  Function_Arg_DataTypes["i16"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["i16"]["1"] = Data_Tree("any");
  Function_Arg_Names["i16"] = {"0", "1"};
  Function_Required_Arg_Count["i16"] = 1;
  // int
  functions_return_data_type["to_int"] = Data_Tree("int");
  Function_Arg_DataTypes["to_int"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["to_int"]["1"] = Data_Tree("any");
  Function_Arg_Names["to_int"] = {"0", "1"};
  Function_Required_Arg_Count["to_int"] = 1;
  // i64
  functions_return_data_type["i64"] = Data_Tree("i64");
  Function_Arg_DataTypes["i64"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["i64"]["1"] = Data_Tree("any");
  Function_Arg_Names["i64"] = {"0", "1"};
  Function_Required_Arg_Count["i64"] = 1;

  // ctz
  functions_return_data_type["ctz"] = Data_Tree("int");
  Function_Arg_DataTypes["ctz"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["ctz"]["1"] = Data_Tree("any");
  Function_Arg_Names["ctz"] = {"0", "1"};
  Function_Required_Arg_Count["ctz"] = 1;

  // swap_bit
  function_return_overwrite["swap_bit"] = swap_bit_ret;
  Function_Arg_DataTypes["swap_bit"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["swap_bit"]["1"] = Data_Tree("any");
  Function_Arg_DataTypes["swap_bit"]["2"] = Data_Tree("int");
  Function_Arg_Names["swap_bit"] = {"0", "1", "2"};
  Function_Required_Arg_Count["swap_bit"] = 2;

  // // simd_load
  function_return_overwrite["simd_load"] = simd_load_ret;

  // vec_shuffle
  function_return_overwrite["vec_shuffle"] = vec_shuffle_ret;

  // vec_make
  function_return_overwrite["vec_make"] = vec_make_ret;
  Function_Arg_DataTypes["vec_make"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["vec_make"]["1"] = Data_Tree("any");
  Function_Arg_DataTypes["vec_make"]["2"] = Data_Tree("int");
  Function_Arg_Names["vec_make"] = {"0", "1", "2"};
  Function_Required_Arg_Count["vec_make"] = 2;

  // vec_movemask
  functions_return_data_type["vec_movemask"] = Data_Tree("int");
  Function_Arg_DataTypes["vec_movemask"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["vec_movemask"]["1"] = Data_Tree("any");
  Function_Arg_Names["vec_movemask"] = {"0", "1"};
  Function_Required_Arg_Count["vec_movemask"] = 1;

  // vec_print
  functions_return_data_type["vec_print"] = Data_Tree("int");
  Function_Arg_DataTypes["vec_print"]["0"] = Data_Tree("Scope_Struct");
  Function_Arg_DataTypes["vec_print"]["1"] = Data_Tree("vec");
  Function_Arg_Names["vec_print"] = {"0", "1"};
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



  // print
  functions_return_data_type["print"] = Data_Tree("void");

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
  Function_Arg_Names["print"] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"};

  set_functions_return_type();
  set_functions_args_type();
  set_user_functions();

  // Prime the first token.
  prebuild();
}

