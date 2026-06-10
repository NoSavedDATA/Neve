#pragma once
#include "../nsk_cpp_llvm.h"

 

Value *simd_add(std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R);
Value *simd_mult(std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R);
Value *simd_equal(std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R);
Value *simd_dif(std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R);
Value *simd_and(std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R);
Value *simd_or(std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R);
Value *simd_minor(std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R);
Value *simd_higher(std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R);
Value *simd_minoreq(std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R);
Value *simd_highereq(std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R);
Value *simd_lshift(std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R);
Value *simd_rshift(std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R);
