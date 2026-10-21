#pragma once

#include "include.h"
#include "../include.h"
#include "memid.h"
#include "lifetime.h"
#include "disown.h"


class TransferChecker {
    public:
        std::unordered_map<int,std::vector<uint64_t>> borrow_ids;
        std::unordered_map<int,std::vector<uint64_t>> borrow_branches;
        std::vector<int> borrow_views;
        std::unordered_map<int,int> borrow_c;
        std::unordered_map<int,uint64_t> memid_to_branch;
        std::vector<std::tuple<int,int,int>> bad_borrows;
        std::string fn, base_fn;

        TransferChecker(std::string, std::string);

        void GetTransfers(Parser_Struct*,ExprAST*,std::unordered_map<std::string, int> &);

        void GetCallMostRestrictive(Parser_Struct *,
            std::string, ExprAST *, ExprAST *,
            std::vector<uint64_t> &,
            int, uint64_t, int,
            std::vector<std::tuple<int,int>> &);


        void HandleAppend(Parser_Struct*, NameableCall *,std::string);
        void HandleCallParams(Parser_Struct*,ExprAST*,std::string);
        void HandleCall(Parser_Struct*,NameableCall*,std::unordered_map<std::string, int> &);


        void HandleNewVec(Parser_Struct*,NewVecExprAST*,std::unordered_map<std::string, int> &);
        void HandleNew(Parser_Struct*,NewExprAST*,std::unordered_map<std::string, int> &);
        void HandleBinOp(Parser_Struct*,BinaryExprAST*,std::unordered_map<std::string, int> &);
        void HandleData(Parser_Struct*,DataExprAST*,std::unordered_map<std::string, int> &);
        void HandleNameable(Parser_Struct*,ExprAST*,Nameable*,std::unordered_map<std::string, int> &);
        void HandleUnkvar(Parser_Struct*,UnkVarExprAST*,std::unordered_map<std::string, int> &);
        void HandleReturn(Parser_Struct*,RetExprAST*,std::unordered_map<std::string, int> &);

        void RegisterMove(Parser_Struct *, uint64_t,
                     ExprAST *, ExprAST *,
                     bool return_on_view=true);

        void CheckBadMove(Parser_Struct *, ExprAST *, ExprAST *);

        void RegisterFn();
};


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


void Disown(Value *scope_struct, Data_Tree &dt, Value *ptr,
        bool is_view=false);




void BorrowChecker(std::string, std::string fn_name,
                   std::unordered_map<std::string, int> &);

