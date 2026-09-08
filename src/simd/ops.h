#pragma once
#include "../nsk_cpp_llvm.h"

 

Value *simd_add(Parser_Struct *, std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R);
Value *simd_mult(Parser_Struct *, std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R);
Value *simd_equal(Parser_Struct *, std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R);
Value *simd_dif(Parser_Struct *, std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R);
Value *simd_and(Parser_Struct *, std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R);
Value *simd_or(Parser_Struct *, std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R);
Value *simd_minor(Parser_Struct *, std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R);
Value *simd_higher(Parser_Struct *, std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R);
Value *simd_minoreq(Parser_Struct *, std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R);
Value *simd_highereq(Parser_Struct *, std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R);
Value *simd_lshift(Parser_Struct *, std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R);
Value *simd_rshift(Parser_Struct *, std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R);
