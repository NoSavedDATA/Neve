#pragma once

#include <string>
#include <vector>

#include "llvm/IR/Value.h"



#include "../compiler_frontend/expressions.h"
#include "../nsk_cpp.h"
#include "../KaleidoscopeJIT.h"
#include "include.h"



using namespace llvm;





Data_Tree min_ret_dt(Parser_Struct, std::vector<std::unique_ptr<ExprAST>>&);
Data_Tree max_ret_dt(Parser_Struct, std::vector<std::unique_ptr<ExprAST>>&);
Data_Tree array_clone_dt(Parser_Struct, std::vector<std::unique_ptr<ExprAST>>&,\
                    std::unique_ptr<Nameable> &);
Data_Tree array_pop_dt(Parser_Struct, std::vector<std::unique_ptr<ExprAST>>&,\
                    std::unique_ptr<Nameable> &);
Data_Tree map_keys_dt(Parser_Struct, std::vector<std::unique_ptr<ExprAST>>&,\
                    std::unique_ptr<Nameable> &);
Data_Tree map_values_dt(Parser_Struct, std::vector<std::unique_ptr<ExprAST>>&,\
                    std::unique_ptr<Nameable> &);
Data_Tree map_get_dt(Parser_Struct, std::vector<std::unique_ptr<ExprAST>>&,\
                    std::unique_ptr<Nameable> &);

Value *DT_charv_Create(Parser_Struct parser_struct, Function *TheFunction,
                      std::string, std::string type, Data_Tree data_type,
                      Value *scope_struct, Value *initial_value,
                      std::vector<std::unique_ptr<ExprAST>> &Args,
                      std::vector<Value*> &ArgsV);
Value *DT_vec_Create(Parser_Struct parser_struct, Function *TheFunction,
                      std::string, std::string type, Data_Tree data_type,
                      Value *scope_struct, Value *initial_value,
                      std::vector<std::unique_ptr<ExprAST>> &Args,
                      std::vector<Value*> &ArgsV);

Value *fexists(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV);
Value *dsize(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV);
Value *printl(Parser_Struct, Function *,
                 std::string, Data_Tree, std::vector<Data_Tree>&,
                 Value *, std::vector<std::unique_ptr<ExprAST>> &, std::vector<Value*>&);
Value *print(Parser_Struct, Function *,
                 std::string, Data_Tree, std::vector<Data_Tree>&,
                 Value *, std::vector<std::unique_ptr<ExprAST>> &, std::vector<Value*>&);
Value *print_bb(Parser_Struct, Function *,
                 std::string, Data_Tree, std::vector<Data_Tree>&,
                 Value *, std::vector<std::unique_ptr<ExprAST>> &, std::vector<Value*>&);
Value *print_Value(Parser_Struct, Function *,
                 std::string, Data_Tree, std::vector<Data_Tree>&,
                 Value *, std::vector<std::unique_ptr<ExprAST>> &, std::vector<Value*>&);
Value *to_char(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV);
Value *i8(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV);
Value *i16(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV);
Value *to_int(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV);
Value *i64(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV);
Value *c_open(Parser_Struct parser_struct, Function *TheFunction,
                 std::string, Data_Tree data_type, std::vector<Data_Tree> &,
                 Value *, std::vector<std::unique_ptr<ExprAST>>&, std::vector<Value*> &);
Value *c_read(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>&, std::vector<Value*> &ArgsV);
Value *err(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>&, std::vector<Value*> &ArgsV);
Value *c_malloc(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>&, std::vector<Value*> &ArgsV);
Value *c_malloc32(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>&, std::vector<Value*> &ArgsV);
Value *c_malloc64(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>&, std::vector<Value*> &ArgsV);
Value *c_malloc_str(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>&, std::vector<Value*> &ArgsV);
Value *c_strlen(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>&, std::vector<Value*> &ArgsV);
Value *str_size(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV);
Value *c_memcpy(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>&, std::vector<Value*> &ArgsV);
Value *c_memchr(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>&, std::vector<Value*> &ArgsV);
Value *str_set(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>&, std::vector<Value*> &ArgsV);
Value *str_offset(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>&, std::vector<Value*> &ArgsV);
Value *alloc(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>&, std::vector<Value*> &ArgsV);
Value *min(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>&, std::vector<Value*> &ArgsV);
Value *max(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>&, std::vector<Value*> &ArgsV);

