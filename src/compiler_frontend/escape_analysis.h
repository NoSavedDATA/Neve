#pragma once
#include "include.h"

extern std::unordered_map<std::string,std::vector<std::tuple<int, Data_Tree, Value*, int>>> OwnedValues;
extern std::map<int, int> ConditionalTakes;
extern std::unordered_map<std::string,std::vector<Value*>> OwnedsCleared;





void Clear_Fn_Owned_Values(Value *scope_struct, Parser_Struct*);


void FreeOwnedPool(Value *scope_struct, Parser_Struct *parser_struct);
void FreeOwnedPoolRet(Value *scope_struct, Parser_Struct *parser_struct,
        std::vector<Value *> &);


void EscapeAnalysis(std::string, std::string fn_name,
                    std::unordered_map<std::string, int> &);


void GetScopeOwnedValues(ExprAST *expr,
        std::vector<std::pair<Data_Tree,Value*>> &owneds);



void SetFnOwn(Parser_Struct *parser_struct, Value*, std::string, std::string fn_name, 
        std::vector<std::unique_ptr<ExprAST>> &Body);
