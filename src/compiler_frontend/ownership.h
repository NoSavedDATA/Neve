#pragma once
#include "include.h"

extern std::vector<std::pair<Data_Tree, Value*>> OwnedValues;



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


void FreeOwnedPool(Value *scope_struct, Parser_Struct *parser_struct);
void FreeOwnedPoolRet(Value *scope_struct, Parser_Struct *parser_struct, bool);


void BorrowChecker(Parser_Struct *parser_struct, std::string fn_name,
            std::vector<std::unique_ptr<ExprAST>> &Body);

void EscapeAnalysis(Parser_Struct *parser_struct, std::string fn_name,
            std::vector<std::unique_ptr<ExprAST>> &Body);


void GetScopeOwnedValues(ExprAST *expr,
        std::vector<std::pair<Data_Tree,Value*>> &owneds);

void Clear_Owned_Values(Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Body);


void SetFnOwn(Parser_Struct *parser_struct, Value*, std::string fn_name, 
        std::vector<std::unique_ptr<ExprAST>> &Body);
