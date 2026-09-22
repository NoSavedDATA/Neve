#pragma once
#include "include.h"




inline bool has_body(ExprAST *expr) {
    if (auto *stmt = dynamic_cast<IfExprAST*>(expr))
        return true;
    if (auto *stmt = dynamic_cast<ForExprAST*>(expr))
        return true;
    if (auto *stmt = dynamic_cast<ForEachExprAST*>(expr))
        return true;
    if (auto *stmt = dynamic_cast<WhileExprAST*>(expr))
        return true;
    if (auto *stmt = dynamic_cast<FinishExprAST*>(expr))
        return true;
    if (auto *stmt = dynamic_cast<AsyncExprAST*>(expr))
        return true;
    if (auto *stmt = dynamic_cast<AsyncsExprAST*>(expr))
        return true;
    if (auto *stmt = dynamic_cast<SpawnExprAST*>(expr))
        return true;
    return false;
}




void BorrowChecker(std::string, std::string fn_name,
                   std::unordered_map<std::string, int> &);

