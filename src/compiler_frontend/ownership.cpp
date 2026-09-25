// Edge cases
//
// -- cannot define memid
//
// if true
//     x = own b()
// else
//     x = own c()
// # Here, memid is compile-time. It cannot branch.
// # Cannot detect whether b or c moves


#include "ownership.h"
#include "expressions.h"
#include "logging.h"
#include "modules.h"
#include "scope.h"
#include <cstdint>
#include <cstdlib>
#include <execution>
#include <iterator>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>




std::map<int, int> ConditionalTakes;



int ExprAST::GetMemId() {
    return -2;
}
int NewExprAST::GetMemId() {
    return MemId;
}
int UnaryExprAST::GetMemId() {
    return Operand->GetMemId();
}
int BinaryExprAST::GetMemId() {
    // return LHS->GetMemId() || RHS->GetMemId();
    int id = RHS->GetMemId();
    if (id>=0) return id;
    id = LHS->GetMemId();
    if (id>=0) return id;
    return -2;
}
int Nameable::GetMemId() {
    // Check Depth 1 owned for borrowed
    if (MemId > -2)
        return MemId;
    if (Depth>1)
        return -2;
    std::string scope = parser_struct->function_name;
    // std::cout << " " << scope << " | " << Name << "\n";
    if (fn_memid[scope].count(Name)==0)
        return -2;
    return fn_memid[scope][Name];
}
int NameableCall::GetMemId() {
    // std::cout << "as call " << MemId << "\n";
    return MemId;
}












void ForExprAST::RegisterOwned(Data_Tree dt, Value *v) {
    OwnedToClear.push_back({dt, v});
}
void ForExprAST::ClearOwned() {
    for(auto &[dt, value] : OwnedToClear) {
        std::cout << "FOR CLEAR " << value << "\n"; 
        dt.Print();
    }
}

void ForEachExprAST::RegisterOwned(Data_Tree dt, Value *v) {
    OwnedToClear.push_back({dt, v});
}
void ForEachExprAST::ClearOwned() {
    for(auto &[dt, value] : OwnedToClear) {
        std::cout << "ForEachExprAST CLEAR " << value << "\n"; 
        dt.Print();
    }
}

void WhileExprAST::RegisterOwned(Data_Tree dt, Value *v) {
    OwnedToClear.push_back({dt, v});
}
void WhileExprAST::ClearOwned() {
    for(auto &[dt, value] : OwnedToClear) {
        std::cout << "While CLEAR " << value << "\n"; 
        dt.Print();
    }
}



void IfExprAST::RegisterOwned(Data_Tree dt, Value *v) {
    OwnedToClear.push_back({dt, v});
}
void IfExprAST::ClearOwned() {
    for(auto &[dt, value] : OwnedToClear) {
        std::cout << "IF CLEAR " << value << "\n"; 
        dt.Print();
    }
}


void BinaryExprAST::RegisterOwned(Data_Tree dt, Value *v) {
    OwnedToClear.push_back({dt, v});
}
void BinaryExprAST::ClearOwned() {
    for(auto &[dt, value] : OwnedToClear) {
        std::cout << "BIRANY CLEAR " << value << "\n"; 
        dt.Print();
    }
}


void NameableCall::RegisterOwned(Data_Tree dt, Value *v) {
    OwnedToClear.push_back({dt, v});
}
void NameableCall::ClearOwned() {
    for(auto &[dt, value] : OwnedToClear) {
        std::cout << "CALL CLEAR " << value << "\n"; 
        dt.Print();
    }
}













int innermost_cstmt(std::string fn_name,
                    int cstmt) {
    if (cstmt_parents.count(fn_name)>0) {
        if (cstmt_parents[fn_name].count(cstmt)>0)
            return cstmt_parents[fn_name][cstmt];
    }
    return cstmt;
}
bool match_cstmt_parent(std::string fn_name,
                    int tgt_cstmt, int cstmt) {
    if (tgt_cstmt==0) // tgt in depth 0 scope
        return true;
    if (cstmt==tgt_cstmt)
        return true;
    if (cstmt_parents.count(fn_name)==0)
        return false;
    if (cstmt_parents[fn_name].count(cstmt)==0)
        return false;
    return match_cstmt_parent(fn_name, tgt_cstmt,
                cstmt_parents[fn_name][cstmt]);
}



void CheckBadBorrow(Parser_Struct *parser_struct, 
                 std::unordered_map<int,std::vector<uint64_t>> &borrow_ids,
                 std::unordered_map<int,int> &borrow_c,
                 std::vector<int> &bad_borrows, ExprAST *nameable
            ) {
    int memid = nameable->GetMemId();
    if (memid==-2)
        return;

    if (borrow_ids.count(memid)) {


        uint64_t expr_branch = nameable->BranchId;
        int expr_control_stmt = (expr_branch>>32)&MASK_16;
        int expr_depth = (expr_branch>>48)&MASK_16;

        for (auto &branch : borrow_ids[memid]) {

            int control_stmt = (branch>>32)&MASK_16;
            int depth = (branch>>48)&MASK_16;
            bool match = (expr_depth>=depth&&match_cstmt_parent(
                        parser_struct->function_name,
                    control_stmt, expr_control_stmt));

            if (!match)  {
                // std::cout << "\ncstmt " << control_stmt << " | " << expr_control_stmt << "\n";
                // std::cout << "depth " << depth << " | " << expr_depth << "\n";
                // std::cout << "match " << (expr_depth>=depth) << " | " << match_cstmt_parent(
                //             parser_struct->function_name,
                //         control_stmt, expr_control_stmt) << "\n";
                // std::cout << parser_struct->function_name << "\n\n";

                bad_borrows.push_back(memid);
            }
        }
    } 
}



uint64_t FormatLifetime(uint64_t branch, int memid) {
    return (branch&~MASK_16) | (uint16_t)memid;
}

uint64_t GetLifetime(Parser_Struct *parser_struct,
        Nameable *owner_expr, ExprAST *borrowed,
        std::unordered_map<int,uint64_t> &memid_to_branch) {
    if (owner_expr->Depth!=1)
        return GetLifetime(parser_struct, owner_expr->Inner.get(),
                           borrowed, memid_to_branch);
    int memid = fn_memid[parser_struct->function_name][owner_expr->Name];


    uint64_t lifetime = (memid_to_branch.count(memid)>0)
        ? memid_to_branch[memid]
        : memid; // is function arg

    uint64_t cap = (borrowed->BranchId>>16) & MASK_16;
    return (lifetime & ~(MASK_16<<16)) | (cap<<16);
    // std::cout << "cap: " << other << " to " << lifetime << "\n";
    // return lifetime;
}



void DataExprAST::SetMemId(std::unordered_map<int,uint64_t> &memid_to_branch) {
  if (!data_type.IsFromArena())
      return;

  for(auto &[name, expr] : this->VarNames) {
    if (IsOwned&&dynamic_cast<NullPtrExprAST*>(expr.get())) {
        int memid = *parser_struct->mem_id;
        memid_to_branch[memid] = FormatLifetime(BranchId, memid);
        fn_memid[parser_struct->function_name][name] = (*parser_struct->mem_id)++;
    } else {
        int memid = expr->GetMemId();
        if (memid > -2)
            fn_memid[parser_struct->function_name][name] = memid;
    }
  }
}


inline void RegisterBorrow(Parser_Struct *parser_struct,
             uint64_t parent_branch,
             std::unique_ptr<ExprAST> &expr,
             std::unordered_map<int,std::vector<uint64_t>> &borrow_ids,
             std::unordered_map<int,int> &borrow_c
         ) {
    int appended_memid = expr->GetMemId();
    // std::cout << "==try register " << parser_struct->function_name << " | " << appended_memid << "\n"; 
    if (appended_memid == -2)
        return;

    // std::cout << "==========REGISTER " << parser_struct->function_name << " | " << appended_memid << "\n"; 

    borrow_ids[appended_memid].push_back(parent_branch);
    borrow_c[appended_memid]++;
}


void GetCallMostRestrictive(Parser_Struct *parser_struct,
            std::string callee,
            NameableCall *callexpr,
            std::vector<uint64_t> &arg_parents,
            int arg_memid, uint64_t callexpr_branch,
            int memid,
            std::unordered_map<int, std::vector<uint64_t>> &borrow_ids) {
    // Try borrow to most restrict caller owner.
    // Else, use call expr as the lifetime.
    int borrows = fn_borrows[callee][arg_memid].size();
    uint64_t fn_owner_branch = fn_borrows[callee][arg_memid][0];
    uint64_t callbranch = callexpr->BranchId;

    int cap = (fn_owner_branch>>16)&MASK_16;

    // std::cout << "cap: " << callee << "->" << fn_borrows_c[callee][arg_memid] << "|" << cap << "\n";
    // std::cout << " " << in_vec(arg_memid,fn_borrows_incomplete[callee]) << "\n";


    bool has_incomplete = fn_borrows_c[callee][arg_memid]<cap
                            ||in_vec(arg_memid,fn_borrows_incomplete[callee]);
    if (has_incomplete)
        fn_borrows_incomplete[parser_struct->function_name].push_back(memid);

    // std::cout << "register borrow " << parser_struct->function_name << "|" <<memid << "\n";
    // TODO: tackle nested if borrow
    if (arg_parents.size()==0||has_incomplete) {
        borrow_ids[memid].push_back(callexpr_branch);
        return;
    }

    uint64_t first_branch = arg_parents[0];
    int cstmt = (first_branch>>32) & MASK_16;
    borrow_ids[memid].push_back(first_branch);
    int most_restrictive = 0;
    for (int i=1; i<arg_parents.size(); ++i) {
        uint64_t branch = arg_parents[i];

        borrow_ids[memid].push_back(branch);

        int cstmt_i = (branch>>32) & MASK_16;
        if (cstmt_i>cstmt)
            most_restrictive=i;
    }
    // std::cout << "most restrictive " << most_restrictive << " | " << arg_parents.size()<< "\n";

    return;
}

void RegisterCallBorrow(Parser_Struct *parser_struct,
            NameableCall *callexpr, std::string callee,
            std::unordered_map<int,std::vector<uint64_t>> &borrow_ids,
            std::unordered_map<int,int> &borrow_c,
            std::vector<int> &bad_borrows,
            std::unordered_map<int,uint64_t> &memid_to_branch,
            std::unordered_map<std::string, int> &seen) {
    std::string fn_name = parser_struct->function_name;
    std::string base_callee = callexpr->BaseCallee;
    // if (fn_argnames.count(callee))
        // std::cout << "Failed for " << callee << "\n";
    if (in_vec(callee,native_methods))
        return; // skip llvm defined


    CallArgsTy &CArgs = callexpr->CArgs;

    std::vector<std::string> argnames;
    bool has_borrow = false;

    std::unordered_map<int, uint64_t> arg_to_caller_memid;

    int i=0,j=0;
    for (auto &argexpr : callexpr->Args) {
        i++;
        j++;

        if (fn_argnames.count(callee)==0)
            return;
        std::string argname = fn_argnames[callee][i-1];
        if (argname=="scope_struct")
            argname = fn_argnames[callee][i++];
        argnames.push_back(argname);

        if (auto *nameableexpr =
                dynamic_cast<Nameable*>(argexpr.get())) {
            if (nameableexpr->Depth!=1) 
                continue;

            if (nameableexpr->GetMemId()==-2)
                continue;

            uint64_t parent_memid = GetLifetime(parser_struct,
                    nameableexpr, callexpr, memid_to_branch);


            int arg_memid = fn_arg_memid[callee][argname];
            arg_to_caller_memid[arg_memid] = parent_memid;

            // std::cout << " " << argname << " | " << arg_memid << "\n"; 
        }
    }


    i=0;
    j=0;
    for (auto &argexpr : callexpr->Args) {
        i++;
        j++;

        if (fn_argnames.count(callee)==0)
            return;
        std::string argname = fn_argnames[callee][i-1];
        if (argname=="scope_struct")
            argname = fn_argnames[callee][i++];

        if (auto *nameableexpr =
                dynamic_cast<Nameable*>(argexpr.get())) {
            if (nameableexpr->Depth!=1) 
                continue;

            int memid = nameableexpr->GetMemId();
            if (memid==-2)
                continue;

            int arg_memid = fn_arg_memid[callee][argname];



            // std::cout << " " << callee << "|" << arg_memid << " | " <<fn_borrows[base_callee].count(arg_memid) << "\n"; 
            if (fn_borrows[base_callee].count(arg_memid)>0) {

                std::vector<uint64_t> caller_parents;
                for (auto &borrow : fn_borrows[base_callee][arg_memid]) {
                    int owner_memid = borrow&MASK_16;
                    if (arg_to_caller_memid.count(owner_memid)) {
                        caller_parents.push_back(
                            arg_to_caller_memid[owner_memid]
                        );
                    }
                }


                GetCallMostRestrictive(parser_struct,
                            base_callee,
                            callexpr,
                            caller_parents,
                            arg_memid, callexpr->BranchId,
                            memid, borrow_ids);
                borrow_c[memid]++;


                Data_Tree &dt = CArgs.dts[j-1];
                int ownid = nameableexpr->GetIsOwned();

                bool is_owned = ownid!=-2||dt.is_borrow||dt.is_own;
                if (is_owned) {
                    has_borrow=true;
                    dt.is_own=false;
                    dt.is_borrow=true;
                    CArgs.borrows.push_back({
                            dt, argname, arg_memid
                        });
                }
            }
        }


    }

    if (has_borrow) {
        CArgs.args = argnames;
        CArgs.template_ret = fn_ret_dt[callee];
        bool found = false;

        callee = GetFnVersion(parser_struct, base_callee, CArgs, found, true, true);
        if (!found) {
            Template_FnAST[base_callee][CArgs] = TheJIT->fn_map[base_callee];
            callee = GenTemplate(parser_struct, base_callee, CArgs, found);
        }
        callexpr->Callee = callee;
        // BorrowChecker(base_callee, callee, seen);
    }
}








void GetBorrows(Parser_Struct *parser_struct,
                std::unordered_map<std::string, int> &seen,
                ExprAST *expr,
                std::unordered_map<int,std::vector<uint64_t>> &borrow_ids,
                std::unordered_map<int,int> &borrow_c,
                std::vector<int> &bad_borrows,
                std::unordered_map<int,uint64_t> &memid_to_branch,
                std::vector<int> &retids, int &retcount
             ) {

    if (auto *callexpr = dynamic_cast<NameableCall*>(expr)) {
        std::string callee = callexpr->Callee;
        std::string base_callee = callexpr->BaseCallee;

        BorrowChecker(base_callee, callee, seen);

        if (callee=="array_append") {
            RegisterBorrow(parser_struct,
                            GetLifetime(parser_struct, (Nameable*)callexpr,
                                callexpr->Args[0].get(), memid_to_branch),
                           callexpr->Args[0],
                           borrow_ids, borrow_c);
            return;
        }
        RegisterCallBorrow(parser_struct,
                callexpr, callee, borrow_ids, borrow_c,
                bad_borrows, memid_to_branch, seen);
        return;
    }


    if (auto *nameable = dynamic_cast<Nameable*>(expr)) {
        CheckBadBorrow(parser_struct, borrow_ids, borrow_c,
                       bad_borrows, nameable);
        int memid = nameable->GetMemId();
        if (!nameable->IsAttr&&memid!=-2) {
            uint64_t branch = nameable->BranchId;
            branch = (branch&~MASK_16) | memid;
            fn_memid_to_lastseen[parser_struct->function_name][memid] = expr;
        }

        return;
    }

    if (auto *dataexpr = dynamic_cast<DataExprAST*>(expr)) {
        dataexpr->SetMemId(memid_to_branch);
        return;
    }
    if (auto *dataexpr = dynamic_cast<UnkVarExprAST*>(expr)) {
        for(auto &[name, _] : dataexpr->VarNames) {
            if (fn_memid[parser_struct->function_name].count(name)) {
                int memid = fn_memid[parser_struct->function_name][name];
                // std::cout << "UNK " << name << "->"<< memid << "\n";
                memid_to_branch[memid] = FormatLifetime(dataexpr->BranchId, memid);
            }
        }
        return;
    }

    if (auto *objexpr = dynamic_cast<ObjectExprAST*>(expr)) {
        for (unsigned i = 0, e = objexpr->VarNames.size(); i != e; ++i) {
            if (!objexpr->HasInit[i]) {
                std::string name = objexpr->VarNames[i].first;
                int memid = fn_memid[parser_struct->function_name][name];
            }
        }
        return;
    }

    if (auto *newexpr = dynamic_cast<NewExprAST*>(expr)) {
        newexpr->MemId = (*parser_struct->mem_id)++;
        return;
    }


    if (auto *ret_expr = dynamic_cast<RetExprAST*>(expr)) {
        for (auto &var : ret_expr->Vars) {
            retcount++;

            int memid = var->GetMemId();
            if (memid>=-1) {
                retids.push_back(memid);
            }
        }
    }
}





void BorrowChecker(std::string base_callee, std::string fn_name,
                    std::unordered_map<std::string, int> &seen) {
    fn_name = base_callee;
    if (in_vec(base_callee, native_fn)
            ||fn_borrows.count(fn_name)>0
            ||seen.count(fn_name)>0
            ||!TheJIT->fn_map.count(base_callee))
        return; // skip llvm fn
    // std::cout << "BorrowChecker " << base_callee << "|" << fn_name << "\n";

    seen[fn_name] = 1;
    FunctionAST *fn_ast = TheJIT->fn_map[base_callee];
    std::vector<std::unique_ptr<ExprAST>> &Body = fn_ast->Body;
    Parser_Struct *parser_struct = fn_ast->parser_struct;
    std::unordered_map<int,std::vector<uint64_t>> borrow_ids;
    std::unordered_map<int,int> borrow_c;
    std::unordered_map<int,uint64_t> memid_to_branch;
    std::vector<int> retids, bad_borrows;
    int retcount=0;
    parser_struct->function_name = fn_name;


    *parser_struct->mem_id = parser_struct->memid_arg_offset;

    for (auto &body : Body) {
      body->Traverse([parser_struct, &seen,
              &borrow_ids, &borrow_c, &bad_borrows, &memid_to_branch,
              &retids, &retcount](ExprAST *node) {

        GetBorrows(parser_struct, seen, node,
                    borrow_ids,
                    borrow_c,
                    bad_borrows,
                    memid_to_branch,
                    retids, retcount
                );
      });
    }

    fn_borrows[fn_name] = borrow_ids;
    fn_borrows_c[fn_name] = borrow_c;
    fn_bad_borrows[fn_name] = bad_borrows;
    fn_rets[fn_name] = retids;
    fn_retscount[fn_name] = retcount;
    for(auto &[memid, branch] : memid_to_branch)
        fn_memid_to_branch[fn_name][memid] = branch;


    for(auto &[memid,borrows]: borrow_ids) {
        uint64_t fn_owner_branch = borrows[0];
        int cap = (fn_owner_branch>>16)&MASK_16;

        bool has_incomplete = borrow_c[memid]<cap;
        if (has_incomplete)
            fn_borrows_incomplete[fn_name].push_back(memid);
    }
}
