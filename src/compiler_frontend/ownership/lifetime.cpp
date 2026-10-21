
#include "../include.h"


uint64_t FormatLifetime(uint64_t branch, int memid) {
    return (branch&~MASK_16) | (uint16_t)memid;
}


uint64_t GetLifetime(Parser_Struct *parser_struct,
        ExprAST *borrowed, std::unordered_map<int,uint64_t> &memid_to_branch) {

    if (auto *newvec_expr = dynamic_cast<NewVecExprAST*>(borrowed)) {
        // std::cout << "AS NEW VEC " << "\n";
        return FormatLifetime(newvec_expr->BranchId, newvec_expr->Memids[0]);
    }
    if (auto *newvec_expr = dynamic_cast<StringExprAST*>(borrowed)) {
        // std::cout << "AS NEW String " << "\n";
        return FormatLifetime(newvec_expr->BranchId, newvec_expr->MemId);
    }

    if (auto *nameable=dynamic_cast<Nameable*>(borrowed)) {
        if (nameable->Depth!=1)
            return GetLifetime(parser_struct, nameable->Inner.get(),
                               memid_to_branch);
    }
    // int memid = fn_memid[parser_struct->base_name][borrowed->Name];
    int memid = (borrowed->Name=="self")
        ? 1
        : fn_memid[parser_struct->base_name][borrowed->Name];
    if (auto *borrowed_nameable = dynamic_cast<Nameable*>(borrowed)) {
        if (borrowed_nameable->IsUnique||borrowed_nameable->IsOwnedUnique) {
            memid = borrowed->GetMemId()[0];
        }
    } 


    uint64_t lifetime = (memid_to_branch.count(memid)>0)
        ? memid_to_branch[memid]
        : memid; // is function arg

    uint64_t cap = (borrowed->BranchId>>16) & MASK_16;
    return (lifetime & ~(MASK_16<<16)) | (cap<<16);
}



uint64_t GetLifetime(Parser_Struct *parser_struct,
        ExprAST *owner_expr, ExprAST *borrowed,
        std::unordered_map<int,uint64_t> &memid_to_branch,
        bool verbose) {

    if (auto *newvec_expr = dynamic_cast<NewVecExprAST*>(borrowed)) {
        return FormatLifetime(newvec_expr->BranchId, newvec_expr->Memids[0]);
    }
    if (auto *newvec_expr = dynamic_cast<StringExprAST*>(borrowed)) {
        return FormatLifetime(newvec_expr->BranchId, newvec_expr->MemId);
    }

    if (auto *owner_nameable = dynamic_cast<Nameable*>(owner_expr)) {
        if (owner_nameable->Depth!=1)
            return GetLifetime(parser_struct, owner_nameable->Inner.get(),
                               borrowed, memid_to_branch,verbose);
    }


    int memid = (owner_expr->Name=="self")
        ? 1
        : fn_memid[parser_struct->base_name][owner_expr->Name];
    if (auto *owner_nameable = dynamic_cast<Nameable*>(owner_expr)) {
        if (owner_nameable->IsUnique||owner_nameable->IsOwnedUnique) {
            memid = owner_expr->GetMemId()[0];
        }
    } 
    // int memid = owner_expr->GetMemId()[0];


    uint64_t lifetime = (memid_to_branch.count(memid)>0)
        ? memid_to_branch[memid]
        : memid; // is function arg or self

    if (verbose) {
        std::cout << "lifetime of " << borrowed->GetName() << "\n";
        std::cout << "owner " << parser_struct->function_name<<"|"<<owner_expr->Name << "\n";
        std::cout << "has " << (memid_to_branch.count(memid)>0) << "\n";
        std::cout << "life " << lifetime << "\n";
    }


    uint64_t cap = (borrowed->BranchId>>16) & MASK_16;
    return (lifetime & ~(MASK_16<<16)) | (cap<<16);
}





int innermost_cstmt(std::string fn_name,
                    int cstmt) {
    if (cstmt_parents.count(fn_name)>0) {
        if (cstmt_parents[fn_name].count(cstmt)>0)
            return cstmt_parents[fn_name][cstmt];
    }
    return cstmt;
}

bool match_cstmt_has_loop(std::string fn_name,
                    int tgt_cstmt, int cstmt) {
    if (cstmt==tgt_cstmt)
        return false;
    if (fn_loop_stmt[fn_name].count(cstmt)>0)
        return true;
    return match_cstmt_has_loop(fn_name, tgt_cstmt,
                cstmt_parents[fn_name][cstmt]);
}

bool match_cstmt_parent(std::string fn_name,
                    int tgt_cstmt, int cstmt) {
    if (cstmt==tgt_cstmt)
        return true;
    if (tgt_cstmt==0) // tgt in depth 0 scope
        return true; 

    if (cstmt_parents.count(fn_name)==0)
        return false;
    if (cstmt_parents[fn_name].count(cstmt)==0)
        return false;
    return match_cstmt_parent(fn_name, tgt_cstmt,
                cstmt_parents[fn_name][cstmt]);
}


