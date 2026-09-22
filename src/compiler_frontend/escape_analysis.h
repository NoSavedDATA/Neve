#pragma once
#include "include.h"

extern std::vector<std::pair<Data_Tree, Value*>> OwnedValues;





void FreeOwnedPool(Value *scope_struct, Parser_Struct *parser_struct);
void FreeOwnedPoolRet(Value *scope_struct, Parser_Struct *parser_struct, bool);


void EscapeAnalysis(Parser_Struct *parser_struct, std::string fn_name,
            std::vector<std::unique_ptr<ExprAST>> &Body);


void GetScopeOwnedValues(ExprAST *expr,
        std::vector<std::pair<Data_Tree,Value*>> &owneds);

void Clear_Owned_Values(Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Body);


void SetFnOwn(Parser_Struct *parser_struct, Value*, std::string fn_name, 
        std::vector<std::unique_ptr<ExprAST>> &Body);
