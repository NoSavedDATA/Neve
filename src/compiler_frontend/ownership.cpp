
#include "ownership.h"
#include "expressions.h"
#include "logging.h"
#include "scope.h"
#include <execution>
#include <iterator>
#include <memory>


std::vector<std::pair<Data_Tree, Value*>> OwnedValues;

void Clear_Fn_Owned_Values(Value *scope_struct) {
    Value *previous_obj = get_scope_obj(scope_struct);
    for (auto &[dt, ptr] : OwnedValues) {
        std::string disown_method = dt.Type+"_disown";
        if (functions_return_data_type.count(disown_method)>0) {
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
        if (functions_return_data_type.count(disown_method)>0)
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
    if (OwnedPoolOffset>=0)
        return OwnedId;
    if (function_callee_escapes.count(Callee)>0)
        return -1;
    return -2;
}



void EscapeAnalysisRecursive(Parser_Struct *parser_struct, std::string fn_name) {
    if (TheJIT->fn_map.count(fn_name)==0)
        LogError(parser_struct->line, "Escape analysis failed for " + fn_name);
    auto &body = TheJIT->fn_map[fn_name]->Body;
    EscapeAnalysis(parser_struct,fn_name,body);
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
        // owned_ids.push_back(callexpr->OwnedId);
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
    if(!functions_return_data_type[fn_name].IsFromArena())
        return;
    if (function_own_ret_count.count(fn_name)>0)
        return;


    std::vector<int> owned_ids, owned_callee_ids;
    int new_ret=0, own_ret_count=0;

    for (auto &body : Body) {
      body->Traverse([parser_struct, &owned_ids, &owned_callee_ids, &own_ret_count, &new_ret](ExprAST *node) {
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
