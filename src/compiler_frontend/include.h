#pragma once


#include <map>
#include <string>
#include <vector>

#include "../nsk_cpp.h"
#include "tokenizers/include.h"
#include "parsers/include.h"
#include "codegen.h"
#include "expressions.h"
#include "function_ast.h"
#include "libs_parser.h"
#include "logging.h"
#include "modules.h"
#include "parser.h"
#include "tokenizer.h"

// #include "../threads/include.h"
#include "../KaleidoscopeJIT.h"




extern std::map<std::string, Data_Tree> Idx_Fn_Return;


extern std::vector<std::string> Sys_Arguments;
 

extern std::map<std::string, std::map<std::string, std::unique_ptr<ExprAST>>> ArgsInit;

extern std::vector<std::string> imported_libs;
extern std::map<std::string, std::vector<std::string>> lib_submodules;

extern std::map<std::string, std::string> lib_function_remaps;





extern std::map<std::string, std::map<std::string, Data_Tree>> data_typeVars;
extern std::map<std::string, std::map<std::string, std::string>> typeVars;

extern std::map<std::string, std::string> floatFunctions;
extern std::map<std::string, std::string> stringMethods;






extern std::unique_ptr<KaleidoscopeJIT> TheJIT;
extern ExitOnError ExitOnErr;

extern PointerType *floatPtrTy, *int8PtrTy, *int1PtrTy;
extern llvm::Type *floatTy, *halfTy, *bf16Ty, *intTy, *int8Ty, *int16Ty, *int64Ty, *m256Ty, *boolTy, *voidTy;

extern Value *stack, *stack_top_value, *cur_self;



extern std::unordered_map<std::string, std::function<Data_Tree(Parser_Struct, std::vector<std::unique_ptr<ExprAST>>&)>>function_return_overwrite;

extern std::unordered_map<std::string, std::function<Data_Tree(Parser_Struct, std::vector<std::unique_ptr<ExprAST>>&, std::unique_ptr<Nameable> &inner)>>method_return_overwrite;
