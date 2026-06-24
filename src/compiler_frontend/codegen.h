#pragma once

#include <string>
#include <vector>

#include "llvm/IR/Value.h"



#include "../nsk_cpp.h"
#include "../KaleidoscopeJIT.h"
#include "include.h"
#include "modules.h"
#include "expressions.h"



using namespace llvm;



extern std::map<std::string, int> fn_stack_offset;
extern std::vector<Value *> thread_pointers;
extern std::map<std::string, std::map<std::string, AllocaInst *>> function_allocas;
extern std::map<std::string, std::map<std::string, Value *>> function_values;
extern std::map<std::string, std::map<Value *, Value *>> function_vecs;
extern std::map<std::string, std::map<std::string, Value *>> function_pointers;
extern std::unordered_map<std::string, llvm::Type*> str_toTy;
extern std::string current_codegen_function;



extern bool seen_var_attr;


Value *VoidPtr_toValue(void *vec);

Function *getFunction(std::string Name);


AllocaInst *CreateEntryBlockAlloca(Function *TheFunction,
                                          StringRef VarName, llvm::Type *);




std::string mangle_cargs_proto(std::string fn_name, bool inc=false);

Value *load_alloca(std::string name, std::string type, std::string from_function);

void StoreVal(Function *TheFunction, std::string fn_name, std::string name, Value *val, Data_Tree dt);
Value *LoadVal(std::string fn_name, std::string name, Data_Tree dt);



Type *get_type_from_str(std::string type);
llvm::Type *get_type_from_data(Data_Tree);

std::string Get_Nested_Name(std::vector<std::string>, Parser_Struct, bool);


bool Check_Is_Compatible_Data_Type(Data_Tree LType, Data_Tree RType, Parser_Struct parser_struct);

bool CheckIsEquivalent(std::string LType, std::string RType);

bool CheckIsSenderChannel(std::string Elements, Parser_Struct parser_struct, std::string LName);


void Allocate_On_Pointer_Stack(Value *, std::string, std::string, Data_Tree, Value *);
void Set_Stack_Top(Value *, std::string);
Value *Load_Pointer_Stack(Value *scope_struct, std::string function_name, std::string var_name);
void Set_Pointer_Stack(Value *scope_struct, std::string function_name, std::string var_name, Value *val);
Value *Load_Stack(Value *scope_struct, const std::string &function_name, const std::string &var_name, const std::string &type);


// void Cache_Array(Parser_Struct, Value *var);
