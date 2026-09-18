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
#include <execution>
#include <iterator>
#include <memory>
#include <string>


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





bool match_cstmt_parent(std::string fn_name,
                    int tgt_cstmt, int cstmt) {
    if (cstmt==tgt_cstmt)
        return true;
    if (cstmt_parents.count(fn_name)==0)
        return false;
    if (cstmt_parents[fn_name].count(cstmt)==0)
        return false;
    return match_cstmt_parent(fn_name, tgt_cstmt,
                cstmt_parents[fn_name][cstmt]);
}








void EscapeAnalysisRecursive(Parser_Struct *parser_struct, std::string fn_name) {
    if (TheJIT->fn_map.count(fn_name)==0)
        LogError(parser_struct->line, "Escape analysis failed for " + fn_name);
    auto &body = TheJIT->fn_map[fn_name]->Body;
    EscapeAnalysis(parser_struct,fn_name,body);
}


// borrow types: 
// 0 - standard
// 1 - to other function

inline void RegisterBorrow(Parser_Struct *parser_struct,
             std::unique_ptr<ExprAST> &expr,
             int borrow_type,
             std::unordered_map<int,std::vector<uint64_t>> &borrow_ids
         ) {

    int appended_memid = expr->GetMemId();
    uint64_t branch = (borrow_type==0)
                    ? expr->BranchId
                    : 0;
    
    if (appended_memid> -2)
        borrow_ids[appended_memid].push_back(branch);

    if (auto *nameable = dynamic_cast<Nameable*>(expr.get())) {
        if (nameable->Depth==1) {
            std::string name = nameable->Name;
        }
    }
}


void RegisterCallBorrow(Parser_Struct *parser_struct,
            NameableCall *callexpr, std::string callee,
            std::unordered_map<int,std::vector<uint64_t>> &borrow_ids) {
    int i=0;
    std::string fn_name = parser_struct->function_name;
    // if (fn_argnames.count(callee))
        // std::cout << "Failed for " << callee << "\n";


    for (auto &argexpr : callexpr->Args) {
        i++;
        if (auto *nameableexpr =
                dynamic_cast<Nameable*>(argexpr.get())) {
            if (nameableexpr->Depth!=1) 
                continue;

            if (fn_argnames.count(callee)==0)
                return;
            std::string argname = fn_argnames[callee][i-1];

            if (argname=="scope_struct")
                argname = fn_argnames[callee][i++];
            int memid = nameableexpr->GetMemId();
            if (memid==-2)
                continue;

            int arg_memid = fn_arg_memid[callee][argname];

            if (fn_borrows[callee].count(arg_memid)>0) {
                borrow_ids[memid]
                    .push_back(
                        (fn_borrows[fn_name].count(memid)>0)
                            ? 1 : 0
                            // had: mark as 1 and throw cannot borrow borrewd
                            // else, let pass normally
                    );
            }
        }
    }
}


void CheckBadBorrow(Parser_Struct *parser_struct, 
                 std::unordered_map<int,std::vector<uint64_t>> &borrow_ids,
                 std::vector<int> &bad_borrows, ExprAST *nameable, int memid
            ) {

    if (borrow_ids.count(memid)) {
        uint64_t branch = borrow_ids[memid][0];
        uint64_t expr_branch = nameable->BranchId;

        int control_stmt = (branch>>32)&MASK_16;
        int expr_control_stmt = (expr_branch>>32)&MASK_16;

        int depth = (branch>>48)&MASK_16;
        int expr_depth = (expr_branch>>48)&MASK_16;

        
        bool match = (expr_depth>=depth&&match_cstmt_parent(
                    parser_struct->function_name,
                control_stmt, expr_control_stmt));

        if (!match)
            bad_borrows.push_back(memid);
    } 
}





void GetBorrows(Parser_Struct *parser_struct, ExprAST *expr,
                 std::unordered_map<int,std::vector<uint64_t>> &borrow_ids,
                 std::vector<int> &bad_borrows,
                 std::vector<int> &retids, int &retcount
                 ) {
    if (auto *callexpr = dynamic_cast<NameableCall*>(expr)) {
        std::string callee = callexpr->Callee;

        if (callee=="append") {
            RegisterBorrow(parser_struct, callexpr->Args[0], 0,
                           borrow_ids);
            return;
        }
        RegisterCallBorrow(parser_struct, callexpr, callee, borrow_ids);
        return;
    }


    if (auto *nameable = dynamic_cast<Nameable*>(expr)) {
        int memid = nameable->GetMemId();
        if (memid==-2)
            return;
        CheckBadBorrow(parser_struct, borrow_ids,
                       bad_borrows, nameable, memid);
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

void BorrowChecker(Parser_Struct *parser_struct, std::string fn_name,
            std::vector<std::unique_ptr<ExprAST>> &Body) {
    std::unordered_map<int,std::vector<uint64_t>> borrow_ids;
    std::vector<int> retids, bad_borrows;
    int retcount=0;


    for (auto &body : Body) {
      body->Traverse([parser_struct,
              &borrow_ids, &bad_borrows, 
              &retids, &retcount](ExprAST *node) {
        GetBorrows(parser_struct, node,
                    borrow_ids,
                    bad_borrows,
                    retids, retcount
                );
      });
    }

    fn_borrows[fn_name] = borrow_ids;

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
