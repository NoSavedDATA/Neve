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
#include "scope.h"
#include <cstdint>
#include <cstdlib>
#include <execution>
#include <iterator>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>


#define MASK_16 0xFFFFULL

std::vector<std::pair<Data_Tree, Value*>> OwnedValues;

void Clear_Fn_Owned_Values(Value *scope_struct) {
    Value *previous_obj = get_scope_obj(scope_struct);
    for (auto &[dt, ptr] : OwnedValues) {
        std::string disown_method = dt.Type+"_disown";
        if (fn_ret_dt.count(disown_method)>0) {
            set_scope_obj(scope_struct, ptr);
            call(disown_method, {scope_struct, ptr});
        }
    }
    set_scope_obj(scope_struct, previous_obj);
}


void GetScopeOwnedValues(ExprAST *expr,
        std::vector<std::pair<Data_Tree,Value*>> &owneds) {
    if(has_body(expr)) return;

    if (auto *new_expr = dynamic_cast<NewExprAST*>(expr)) {
        if (new_expr->IsOwn)
            owneds.push_back({Data_Tree(new_expr->DataName), new_expr->ptr});
    }
}


void Clear_Owned_Values(Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Body) {
    std::vector<std::pair<Data_Tree,Value*>> owneds;
    for (auto &body : Body) {
        body->Traverse([&owneds](ExprAST *node) {
            GetScopeOwnedValues(node, owneds);
        });
    }

    Value *previous_obj = get_scope_obj(scope_struct);
    for (auto &[dt, ptr] : owneds) {
        set_scope_obj(scope_struct, ptr);
        std::string disown_method = dt.Type+"_disown";
        if (fn_ret_dt.count(disown_method)>0)
        call(disown_method, {scope_struct, ptr});
    }
    set_scope_obj(scope_struct, previous_obj);
}




void FreeOwnedPool(Value *scope_struct, Parser_Struct *parser_struct) {
  if (!parser_struct||!scope_struct)
    return;
  if (!parser_struct->has_own())
    return;

  call("free", {
    get_scope_owned_pool(scope_struct)
  });
}

void FreeOwnedPoolRet(Value *scope_struct, Parser_Struct *parser_struct, bool clear_owned) {
  if (clear_owned)
    Clear_Fn_Owned_Values(scope_struct);
  FreeOwnedPool(scope_struct, parser_struct);
}





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
    if (fn_memid[scope].count(Name)==0)
        return -2;
    return fn_memid[scope][Name];
}
int NameableCall::GetMemId() {
    return MemId;
}







int ExprAST::GetIsOwned() {
    return -2;
}
int NewExprAST::GetIsOwned() {
    return OwnedId;
}
int UnaryExprAST::GetIsOwned() {
    return Operand->GetIsOwned();
}
int BinaryExprAST::GetIsOwned() {
    // return LHS->GetIsOwned() || RHS->GetIsOwned();
    int id = RHS->GetIsOwned();
    if (id>=0) return id;
    id = LHS->GetIsOwned();
    if (id>=0) return id;
    return -2;
}
int Nameable::GetIsOwned() {
    // Check Depth 1 owned for borrowed
    if (Depth>1)
        return -2;
    std::string scope = parser_struct->function_name;
    if (function_owns[scope].count(Name)==0)
        return -2;
    return function_owns[scope][Name];
}
int NameableCall::GetIsOwned() {
    // allocates return
    if (OwnedPoolOffset>=0) 
        return OwnedId;
    // returns into reserved memory
    if (function_callee_escapes.count(Callee)>0)
        return -1;
    return -2;
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
        // std::cout << "\n\n " << parser_struct->function_name << " | " << memid << "\n";
        uint64_t expr_branch = nameable->BranchId;
        int expr_control_stmt = (expr_branch>>32)&MASK_16;
        int expr_depth = (expr_branch>>48)&MASK_16;

        for (auto &branch : borrow_ids[memid]) {

            int control_stmt = (branch>>32)&MASK_16;
            int depth = (branch>>48)&MASK_16;

            // std::cout << "\ncstmt " << control_stmt << " | " << expr_control_stmt << "\n";
            // std::cout << "depth " << depth << " | " << expr_depth << "\n";
            // std::cout << "match " << (expr_depth>=depth) << " | " << match_cstmt_parent(
            //             parser_struct->function_name,
            //         control_stmt, expr_control_stmt) << "\n";


            


            bool match = (expr_depth>=depth&&match_cstmt_parent(
                        parser_struct->function_name,
                    control_stmt, expr_control_stmt));

            if (!match)
                bad_borrows.push_back(memid);
        }
    } 
}





void EscapeAnalysisRecursive(Parser_Struct *parser_struct, std::string fn_name) {
    if (TheJIT->fn_map.count(fn_name)==0)
        LogError(parser_struct->line, "Escape analysis failed for " + fn_name);
    auto &body = TheJIT->fn_map[fn_name]->Body;
    EscapeAnalysis(parser_struct,fn_name,body);
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
             int borrow_type,
             std::unordered_map<int,std::vector<uint64_t>> &borrow_ids,
             std::unordered_map<int,int> &borrow_c
         ) {

    int appended_memid = expr->GetMemId();
    uint64_t branch = (borrow_type==0)
                    ? parent_branch
                    : 0;
    
    // std::cout << "append  "  << appended_memid << "\n";
    // std::cout << "append  " << expr->BranchId << " | " << parent_branch << "\n";

    if (appended_memid> -2) {
        borrow_ids[appended_memid].push_back(branch);
        borrow_c[appended_memid]++;
    }

    if (auto *nameable = dynamic_cast<Nameable*>(expr.get())) {
        if (nameable->Depth==1) {
            std::string name = nameable->Name;
        }
    }
}


void GetCallMostRestrictive(std::string callee,
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
    // std::cout << "check " << memid << " | " << nameableexpr->Name << "|" << argname << "|" << arg_memid << "\n";

    int cap = (fn_owner_branch>>16)&MASK_16;
    std::cout << "cap: " << callee << "->" << fn_borrows_c[callee][arg_memid] << "|" << cap << "\n";

    if (arg_parents.size()==0||fn_borrows_c[callee][arg_memid]<cap) {
        borrow_ids[memid].push_back(callexpr_branch);
        return;
    }

    uint64_t first_branch = arg_parents[0];
    int cstmt = (first_branch>>32) & MASK_16;
    borrow_ids[memid].push_back(first_branch);
    int most_restrictive = 0;
    for (int i=1; i<arg_parents.size(); ++i) {
        uint64_t branch = arg_parents[i];

        // borrow_ids[memid].push_back((branch&~(MASK_16<<16))|cap<<16);
        borrow_ids[memid].push_back(branch);

        int cstmt_i = (branch>>32) & MASK_16;
        std::cout << "compare " << first_branch << " to " << branch << "\n"; 
        std::cout << "compare " << cstmt << " to " << cstmt_i << "\n"; 
        if (cstmt_i>cstmt)
            most_restrictive=i;
    }
    std::cout << "most restrictive " << most_restrictive << " | " << arg_parents.size()<< "\n";

    return;
}

void RegisterCallBorrow(Parser_Struct *parser_struct,
            NameableCall *callexpr, std::string callee,
            std::unordered_map<int,std::vector<uint64_t>> &borrow_ids,
            std::unordered_map<int,int> &borrow_c,
            std::vector<int> &bad_borrows,
            std::unordered_map<int,uint64_t> &memid_to_branch) {
    std::string fn_name = parser_struct->function_name;
    std::string base_callee = callexpr->BaseCallee;
    // if (fn_argnames.count(callee))
        // std::cout << "Failed for " << callee << "\n";
    if (!in_vec(callee,fn_called))
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
            std::cout << "tie " << argname << " -> " << arg_memid << " | " << parent_memid << "\n";
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


            if (fn_borrows[callee].count(arg_memid)>0) {
                std::vector<uint64_t> caller_parents;
                for (auto &borrow : fn_borrows[callee][arg_memid]) {
                    int owner_memid = borrow&MASK_16;
                    if (arg_to_caller_memid.count(owner_memid)) {
                        std::cout << "TIE " << owner_memid << "-" << arg_memid << "\n"; 
                        caller_parents.push_back(
                            arg_to_caller_memid[owner_memid]
                        );
                    }
                }


                GetCallMostRestrictive(callee,
                            callexpr,
                            caller_parents,
                            arg_memid, callexpr->BranchId,
                            memid, borrow_ids);

                int branch_cap = (borrow_ids[memid][0]>>16)&MASK_16;


                borrow_c[memid]++;

                // borrow_ids[memid].push_back(owner);
                // std::cout << "______ " << memid << " TO " << (owner&MASK_16) << "\n";

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
    }
}








void GetBorrows(Parser_Struct *parser_struct, ExprAST *expr,
                 std::unordered_map<int,std::vector<uint64_t>> &borrow_ids,
                 std::unordered_map<int,int> &borrow_c,
                 std::vector<int> &bad_borrows,
                 std::unordered_map<int,uint64_t> &memid_to_branch,
                 std::vector<int> &retids, int &retcount
             ) {

    if (auto *callexpr = dynamic_cast<NameableCall*>(expr)) {
        std::string callee = callexpr->Callee;
        if (parser_struct->function_name==callee)
            return; // cut recursion
        // std::cout << "fn borrow: " << callee << "\n";

        BorrowChecker(callee);

        if (callee=="array_append") {
            RegisterBorrow(parser_struct,
                            GetLifetime(parser_struct, (Nameable*)callexpr,
                                callexpr->Args[0].get(), memid_to_branch),
                           callexpr->Args[0], 0,
                           borrow_ids, borrow_c);
            return;
        }
        RegisterCallBorrow(parser_struct,
                callexpr, callee, borrow_ids, borrow_c,
                bad_borrows, memid_to_branch);
        return;
    }


    if (auto *nameable = dynamic_cast<Nameable*>(expr)) {
        CheckBadBorrow(parser_struct, borrow_ids, borrow_c,
                       bad_borrows, nameable);
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
                std::cout << "UNK " << name << "->"<< memid << "\n";
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
                std::cout << "NEW  " << name << "\n";
                std::cout << "NEW  " << memid << "\n";
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





void BorrowChecker(std::string fn_name) {
    if (!in_vec(fn_name, fn_called)||fn_borrows.count(fn_name)>0)
        return; // skip llvm fn
    FunctionAST *fn_ast = TheJIT->fn_map[fn_name];
    std::vector<std::unique_ptr<ExprAST>> &Body = fn_ast->Body;
    Parser_Struct *parser_struct = fn_ast->parser_struct;
    std::unordered_map<int,std::vector<uint64_t>> borrow_ids;
    std::unordered_map<int,int> borrow_c;
    std::unordered_map<int,uint64_t> memid_to_branch;
    std::vector<int> retids, bad_borrows;
    int retcount=0;

    std::cout << "BorrowChecker " << fn_name << "\n";

    *parser_struct->mem_id = parser_struct->memid_arg_offset;
    // if (parser_struct->class_name!="") {
    //     std::cout << "for class-> " << parser_struct->class_name << " " << parser_struct->function_name << "\n";
    //     (*parser_struct->mem_id)++;
    // }

    for (auto &body : Body) {
      body->Traverse([parser_struct,
              &borrow_ids, &borrow_c, &bad_borrows, &memid_to_branch,
              &retids, &retcount](ExprAST *node) {

        GetBorrows(parser_struct, node,
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
}


void GetOwnedRet(Parser_Struct *parser_struct, ExprAST *expr,
                 std::vector<int> &owned_ids,
                 std::vector<int> &owned_callee_ids,
                 int &own_ret_count, int &new_ret) {
    if (auto *callexpr = dynamic_cast<NameableCall*>(expr)) {
        std::string callee = callexpr->Callee;
        if (function_own_ret_count.count(callee)==0) {
            if (fn_owns.count(callee)==0)
                return;
            std::cout << "NEEDS RECURSIZE escape analysis" << "\n";
            EscapeAnalysisRecursive(parser_struct, callee);
        }
        owned_callee_ids.push_back(callexpr->OwnedId);
        own_ret_count+=function_own_ret_count[callee];
        return;
    }

    if (auto *ret_expr = dynamic_cast<RetExprAST*>(expr)) {
        for (auto &var : ret_expr->Vars) {
            int owned_id = var->GetIsOwned();

            // todo: can change to >=0?
            if (owned_id>=-1) {
                owned_ids.push_back(owned_id);
                own_ret_count++;
            }
            else if (auto *callexpr = dynamic_cast<NameableCall*>(var.get()))
                continue;
            else
                new_ret++;
            if (owned_ids.size()>0&&ret_expr->Vars.size()>1)
                LogErrorC(-1, "Tuple function return does not yet support owned values.");
        }
    }
}

void EscapeAnalysis(Parser_Struct *parser_struct, std::string fn_name,
            std::vector<std::unique_ptr<ExprAST>> &Body) {
    if(!fn_ret_dt[fn_name].IsFromArena())
        return;
    if (function_own_ret_count.count(fn_name)>0)
        return;


    std::vector<int> owned_ids, owned_callee_ids;
    int new_ret=0, own_ret_count=0;

    for (auto &body : Body) {
      body->Traverse([parser_struct, &owned_ids,
              &owned_callee_ids,
              &own_ret_count, &new_ret](ExprAST *node) {
        GetOwnedRet(parser_struct, node,
                    owned_ids, owned_callee_ids,
                    own_ret_count, new_ret);
      });
    }

    if (own_ret_count>0&&new_ret>0) {
        LogError(parser_struct->line, "Ambiguous return for \"" + fn_name + "\". Cannot return GC arena and owned pointers from the same function.");
    }

    function_escapes[fn_name] = owned_ids;
    function_callee_escapes[fn_name] = owned_callee_ids;
    function_own_ret_count[fn_name] = own_ret_count;
}



void GetOwnedValues(ExprAST *expr, std::string fn_name, int &last_offset) {
    if (auto *new_expr = dynamic_cast<NewExprAST*>(expr)) {
        if (new_expr->IsOwn) {
            new_expr->OwnedPoolOffset = last_offset;
            last_offset += ClassSize[new_expr->DataName];
        }
    }
    if (auto *callexpr = dynamic_cast<NameableCall*>(expr)) {
        std::string callee = callexpr->Callee;

        int owned_id = callexpr->OwnedId;
        if (owned_id < -1||in_vec(owned_id, function_callee_escapes[fn_name]))
            return;
        if (function_own_ret_count.count(callee)==0)
            return;
        int size = function_own_ret_count[callee];
        callexpr->OwnedPoolOffset = last_offset;
        callexpr->OwnedPoolCap = size;
        last_offset += size * ClassSize[callexpr->GetDataTree().Type];
    }
}


void SetFnOwn(Parser_Struct *parser_struct, Value *scope_struct,
        std::string fn_name, 
        std::vector<std::unique_ptr<ExprAST>> &Body) {
    if(!parser_struct->has_own())
        return;

    // Set own pool
    int last_offset=0;
    for (auto &body : Body) {
        body->Traverse([&last_offset, fn_name](ExprAST *node) {
            GetOwnedValues(node, fn_name, last_offset);
        });
    }
    bool has_owned_pool = last_offset>0;

    if (has_owned_pool) {
      Value *ownedpool = callret("malloc", {const_int(last_offset)});
      set_scope_owned_pool(scope_struct, ownedpool);
    }

  for (auto &id : function_escapes[fn_name]) {
    if (id<0)
        continue;
    fn_owned_ret_memory[id] = get_scope_escape_retoffset(scope_struct);
  }

}
