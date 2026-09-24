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


#include "modules.h"
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
        if (new_expr->MemoryType>0)
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
  if (!parser_struct->has_owned_pool)
    return;

  // std::cout << "FrEE POOL " << parser_struct->function_name << "\n";

  call("free", {
    get_scope_owned_pool(scope_struct)
  });
}

void FreeOwnedPoolRet(Value *scope_struct, Parser_Struct *parser_struct, bool clear_owned) {
  if (clear_owned)
    Clear_Fn_Owned_Values(scope_struct);
  FreeOwnedPool(scope_struct, parser_struct);
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
    // std::cout << " " << scope << " | " << Name << ": " << function_owns[scope].count(Name) << "\n";
    if (function_owns[scope].count(Name)==0)
        return -2;
    return function_owns[scope][Name];
}
int NameableCall::GetIsOwned() {
    // allocates return
    if (OwnedPoolOffset>=0||OwnedId > -2)
        return OwnedId;
    // returns into reserved memory
    // if (function_callee_escapes.count(Callee)>0)
    //     return -1;
    return -2;
}






















void GetOwnedValues(ExprAST *expr, Value *scope_struct,
        std::string fn_name, int &last_offset) {
    if (auto *new_expr = dynamic_cast<NewExprAST*>(expr)) {
        int owned_id = new_expr->GetIsOwned();
        if (new_expr->MemoryType>0) {
            if(in_vec(owned_id,function_escapes[fn_name]))
                fn_owned_ret_memory[owned_id] = get_scope_escape_retoffset(scope_struct);
            else if(!fn_borrows[fn_name].count(new_expr->GetMemId())) {
                new_expr->OwnedPoolOffset = last_offset;
                last_offset += ClassSize[new_expr->DataName];
            }
        }
        return;
    }

    if (auto *callexpr = dynamic_cast<NameableCall*>(expr)) {
        std::string callee = callexpr->Callee;
        int owned_id = callexpr->OwnedId;
        
        
        if(owned_id==-2||in_vec(owned_id,function_escapes[fn_name]))
            return;
        int memid = callexpr->MemId;


        if (fn_borrows[fn_name].count(memid)!=0)
            return;



        int size = function_own_ret_count[callee];
        callexpr->OwnedPoolOffset = last_offset;
        callexpr->OwnedPoolCap = size;
        last_offset += size * ClassSize[callexpr->GetDataTree().Type];
        // std::cout << "$ set last_offset: " << fn_name << "|" << callee << "|" <<  last_offset << " -- " << size << "\n";
        return;
    }
}


void SetFnOwn(Parser_Struct *parser_struct, Value *scope_struct,
        std::string fn_name, 
        std::vector<std::unique_ptr<ExprAST>> &Body) {

    if (fn_ret_dt.count(fn_name)>0)
        parser_struct->borrow_ret = fn_ret_dt[fn_name].is_borrow;


    // Set own pool
    int last_offset=0;
    for (auto &body : Body) {
        body->Traverse([&last_offset, &scope_struct, fn_name](ExprAST *node) {
            GetOwnedValues(node, scope_struct, fn_name, last_offset);
        });
    }
    bool has_owned_pool = last_offset>0;

    if (has_owned_pool) {
        parser_struct->has_owned_pool = true;
        // std::cout << "==HAS_owned_pool " << parser_struct->function_name << ", last offset: " << last_offset << "\n";
        Value *ownedpool = callret("malloc", {const_int(last_offset)});
        set_scope_owned_pool(scope_struct, ownedpool);
    }
}




void SetToBorrowedRet(Parser_Struct *parser_struct, 
         NameableCall *callexpr) {
    std::string callee = callexpr->Callee;
    std::string base_callee = callexpr->BaseCallee;
    CallArgsTy CArgs = callexpr->CArgs;
    CArgs.template_ret.is_borrow=true;
    CArgs.template_ret.Print();
    bool found = false;

    std::string prev_callee = callee;
    callee = GetFnVersion(parser_struct, base_callee, CArgs, found, true, true);
    if (!found) {
        std::vector<std::string> argnames;
        for (auto &argname : fn_argnames[callee]) {
            if (argname=="scope_struct")
                continue;
            argnames.push_back(argname);
        }
        CArgs.args = argnames;
         
        FunctionAST *fn_ast = TheJIT->fn_map[base_callee];
        Template_FnAST[base_callee][CArgs] = fn_ast;
        callee = GenTemplate(parser_struct, base_callee, CArgs, found);
        
        for (auto &body : fn_ast->Body) {
          body->TraversePost([parser_struct, &callee](ExprAST *node) {
            if (auto *nestedcall = dynamic_cast<NameableCall*>(node)) {
                std::string nestedcallee = nestedcall->Callee;
                int owned_id =  nestedcall->OwnedId;
                // std::cout << ">> " << nestedcallee << " | " << in_vec(owned_id,function_escapes[callee]) << "\n"; 
                SetToBorrowedRet(parser_struct, nestedcall);
            }
          });
        }
    }
    callexpr->Callee = callee;
}




void GetOwnedRet(Parser_Struct *parser_struct,
                 std::unordered_map<std::string, int> &seen,
                 ExprAST *expr,
                 std::vector<int> &owned_ids,
                 int &own_ret_count, int &new_ret,
                 std::unordered_map<int, int> &ownid_to_size,
                 int &last_ownid) {
    if (auto *callexpr = dynamic_cast<NameableCall*>(expr)) {
        std::string callee = callexpr->Callee;
        EscapeAnalysis(callexpr->BaseCallee, callee, seen);

        if (!function_own_ret_count.count(callee))
            return;

        // std::cout << "-- " << callee << " has " << function_own_ret_count[callee] << "\n"; 
        callexpr->OwnedId = last_ownid;
        ownid_to_size[last_ownid++] = function_own_ret_count[callee];

        if (fn_borrows[parser_struct->function_name].count(callexpr->MemId)>0)
            SetToBorrowedRet(parser_struct, callexpr);

        return;
    }



    // Update ownid in order to encompass calls
    if (auto *stmt = dynamic_cast<BinaryExprAST*>(expr)) {
        if (stmt->Op!='=')
            return;
        if(auto *lhs = dynamic_cast<Nameable*>(stmt->LHS.get())) {

            if(lhs->Depth>1)
                return;
            std::string name = lhs->Name;
            std::string fn = parser_struct->function_name;
            int own_id = stmt->RHS->GetIsOwned();
            if (own_id > -2)
                function_owns[fn][name] = own_id;
        }
    }

    if (auto *stmt = dynamic_cast<DataExprAST*>(expr)) {
      for(auto &[name, expr] : stmt->VarNames) {
            std::string fn = parser_struct->function_name;
            int own_id = expr->GetIsOwned();
            if (own_id > -2)
                function_owns[fn][name] = own_id;
        }
    }
    if (auto *stmt = dynamic_cast<UnkVarExprAST*>(expr)) {
      for(auto &[name, expr] : stmt->VarNames) {
            std::string fn = parser_struct->function_name;
            int own_id = expr->GetIsOwned();
            if (own_id > -2)
                function_owns[fn][name] = own_id;
        }
    }
    if (auto *stmt = dynamic_cast<ObjectExprAST*>(expr)) {
      for(auto &[name, expr] : stmt->VarNames) {
            if (!expr)
                continue;
            std::string fn = parser_struct->function_name;
            int own_id = expr->GetIsOwned();
            if (own_id > -2)
                function_owns[fn][name] = own_id;
        }
    }

    // Ret
    if (auto *ret_expr = dynamic_cast<RetExprAST*>(expr)) {
        for (auto &var : ret_expr->Vars) {
            int owned_id = var->GetIsOwned();


            // todo: can change to >=0?
            if (owned_id>=-1) {
                // std::cout << "add ret " << parser_struct->function_name << " | " << owned_id << "\n"; 
                owned_ids.push_back(owned_id);
                if(ownid_to_size.count(owned_id)) {
                    own_ret_count+=ownid_to_size[owned_id];
                }
                else
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


void EscapeAnalysis(std::string base_callee, std::string fn_name,
                    std::unordered_map<std::string, int> &seen) {
    if (in_vec(base_callee, native_fn)
        ||seen.count(fn_name)>0
        ||!TheJIT->fn_map.count(base_callee))
        return;

    seen[fn_name] = 1;
    FunctionAST *fn_ast = TheJIT->fn_map[base_callee];
    std::vector<std::unique_ptr<ExprAST>> &Body = fn_ast->Body;
    Parser_Struct *parser_struct = fn_ast->parser_struct;
    parser_struct->function_name = fn_name;
    int last_ownid = *parser_struct->owned_id; // set calls ownid

    std::vector<int> owned_ids;
    std::unordered_map<int, int> ownid_to_size;
    int new_ret=0, own_ret_count=0;

    for (auto &body : Body) {
      body->TraversePost([parser_struct, &seen, &owned_ids,
              &own_ret_count, &new_ret,
              &ownid_to_size,
              &last_ownid](ExprAST *node) {
        GetOwnedRet(parser_struct, seen, node,
                    owned_ids,
                    own_ret_count, new_ret,
                    ownid_to_size, last_ownid);
      });
    }

    if (own_ret_count>0&&new_ret>0) {
        LogError(parser_struct->line, "Ambiguous return for \"" + fn_name + "\". Cannot return GC arena and owned pointers from the same function.");
    }



    if (own_ret_count==0)
        return;


    function_escapes[fn_name] = owned_ids;
    function_own_ret_count[fn_name] = own_ret_count;
    // std::cout << " " << fn_name << " has " << owned_ids.size() << " escapes\n"; 
}
