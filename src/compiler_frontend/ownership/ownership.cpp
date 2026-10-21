#include "ownership.h"
#include "include.h"
#include "../include.h"
#include "lifetime.h"
#include <complex>
#include <cstdint>
#include <cstdlib>
#include <execution>
#include <iterator>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>




TransferChecker::TransferChecker(std::string fn, std::string base_fn)
        : fn(fn), base_fn(base_fn) {}


void TransferChecker::CheckBadMove(Parser_Struct *parser_struct, 
                 ExprAST *parent, ExprAST *nameable) {

    if (auto *stmt = dynamic_cast<Nameable*>(nameable)) {
        if (stmt->Depth>1)  {
            std::cout << "\n\t\033[33mTransfer attribute " << stmt->GetName() << "\033[0m\n\n";

        }
        if (stmt->Depth>1&&stmt->Inner->GetIsOwned())  {
            std::cout << "\n\t\033[33mAttr owned " << stmt->GetName() << "\033[0m\n\n";

        }
    }


    int memid = nameable->GetMemId()[0];
    if (memid==OM_non_obj)
        return;
    // std::cout << "check bad_borrow " << parser_struct->function_name << " | " << memid << "\n";

    if (!dynamic_cast<Nameable*>(nameable))
        return;

    

    ExprBranch taken_lifetime = GetLifetime(parser_struct,
        (Nameable*)nameable, memid_to_branch);

    if (borrow_ids.count(memid)) {
        if (parent!=nullptr) {
            if (auto *nameable_parent = dynamic_cast<Nameable*>(parent)) {
                Data_Tree r_dt = nameable->GetDataTree();
                bool bad_rhs = nameable->GetIsOwned()==OM_non_obj
                            && r_dt.IsTraversable();
                if (parent->GetIsOwned()!=OM_non_obj&&bad_rhs) {
                    // assign GC obj to owned
                    bad_borrows.push_back({memid,3,nameable->Line});
                }
            }
        }


        ExprBranch expr_branch = nameable->BranchId;
        int expr_control_stmt = expr_branch.cstmt;
        int expr_depth = expr_branch.depth;
        int line = nameable->Line;

        int i=0;
        ExprBranch first_taken_branch = borrow_branches[memid][0];
        for (auto &lifetime : borrow_ids[memid]) {
            ExprBranch taken_branch = borrow_branches[memid][i++];


            bool has_loop = match_cstmt_has_loop(
                        parser_struct->base_name,
                    taken_lifetime.cstmt, expr_control_stmt);

            int control_stmt = lifetime.cstmt;
            int depth = lifetime.depth;
            bool match = expr_depth>=depth&&(match_cstmt_parent(
                        parser_struct->base_name,
                    control_stmt, expr_control_stmt));

            if (!match)  {
                // std::cout << "\ncstmt " << control_stmt << " | " << expr_control_stmt << "\n";
                // std::cout << "depth " << depth << " | " << expr_depth << "\n";
                // std::cout << "match " << (expr_depth>=depth) << " | " << match_cstmt_parent(
                //             parser_struct->base_name,
                //         control_stmt, expr_control_stmt) << "\n";
                // std::cout << parser_struct->function_name << "\n\n";

                bad_borrows.push_back({memid,0,line});
                return;
            }


            if (i!=0
                &&(first_taken_branch.cstmt != taken_branch.cstmt)) {
                std::cout << "SET BAD BORROW " << memid << "\n";
                bad_borrows.push_back({memid,1,line});
                return;
            }

            if (has_loop)  {
                bad_borrows.push_back({memid,2,line});
                return;
            }
        }
    } 
}






void DataExprAST::SetMemId(std::unordered_map<int,ExprBranch> &memid_to_branch) {
  if (!data_type.IsFromArena())
      return;

  int i=0;
  for(auto &[name, expr] : this->VarNames) {
    int memid = Memids[i++];
    if (IsOwned&&dynamic_cast<NullPtrExprAST*>(expr.get())) {
        memid_to_branch[memid] = FormatLifetime(BranchId, memid);
    } 
  }
}




void TransferChecker::GetCallMostRestrictive(Parser_Struct *parser_struct,
            std::string callee,
            ExprAST *callexpr,
            ExprAST *argexpr,
            std::vector<ExprBranch> &arg_parents,
            int arg_memid, ExprBranch callexpr_branch,
            int memid,
            std::vector<std::tuple<int,int>> &partialtakes) {

    // Try borrow to most restrict caller owner.
    // Else, use call expr as the lifetime.
    ExprBranch fn_owner_branch = fn_borrows[callee][arg_memid][0];
    int cap = fn_owner_branch.cap;

    // std::cout << "-------__>cap: " << callee << "->" << fn_borrows_c[callee][arg_memid] << "|" << cap << "\n";


    bool has_incomplete = fn_borrows_c[callee][arg_memid]<cap
                    ||in_vec(arg_memid,fn_borrows_incomplete[callee]);
    if (has_incomplete) {
        fn_borrows_incomplete[parser_struct->base_name].push_back(memid);
        partialtakes.push_back({memid,arg_memid});
    }


    borrow_branches[memid].push_back(callexpr_branch);


    // transfers, but to something created in the function
    if (arg_parents.size()==0) { 
        borrow_ids[memid].push_back(GetLifetime(
                    parser_struct,callexpr,memid_to_branch)
                );
        return;
    }


    ExprBranch first_branch = arg_parents[0];
    int cstmt = first_branch.cstmt;
    borrow_ids[memid].push_back(first_branch);


    if (begins_with(parser_struct->function_name, "Res"))
    std::cout << "PUSH BORROW  " << parser_struct->function_name << ", memid: " << memid << " to parent " << (first_branch.memid) << ", to call " << (callexpr_branch.memid)<< "\n";

    if ((first_branch.memid)==0)
        LogError(-1, "Parent is impossible to be 0");



    int most_restrictive = 0;
    for (int i=1; i<arg_parents.size(); ++i) {
        ExprBranch branch = arg_parents[i];

        borrow_ids[memid].push_back(branch);
        borrow_branches[memid].push_back(callexpr_branch);

        int cstmt_i = branch.cstmt;
        if (cstmt_i>cstmt)
            most_restrictive=i;
    }

    CheckBadMove(parser_struct, nullptr, argexpr);

    return;
}




void TransferChecker::RegisterMove(Parser_Struct *parser_struct,
             ExprBranch parent_lifetime,
             ExprAST *parent_expr, ExprAST *expr, bool return_on_view
         ) {
    int appended_memid = expr->GetMemId()[0];
    if (appended_memid == OM_non_obj)
        return;
    ExprBranch branch = expr->BranchId;


    if (parent_expr->GetIsView()) {
        if (return_on_view)
            return;
        fn_borrow_views[parser_struct->base_name].push_back(appended_memid);
    }
    // std::cout << "==========REGISTER " << parser_struct->function_name << " | " << appended_memid << " in branch " << (branch.cstmt) << "\n"; 
    borrow_ids[appended_memid].push_back(parent_lifetime);
    borrow_branches[appended_memid].push_back(branch);
    borrow_c[appended_memid]++;
    if (auto *nameableparent = dynamic_cast<Nameable*>(parent_expr))
        parent_expr = nameableparent->Inner.get();
    //else: is a [a,b,c] or other expr

    CheckBadMove(parser_struct, parent_expr, expr);
}





void TransferChecker::HandleAppend(Parser_Struct* parser_struct, NameableCall *callexpr,std::string callee) {
    RegisterMove(parser_struct,
                    GetLifetime(parser_struct, (Nameable*)callexpr,
                        callexpr->Args[0].get(), memid_to_branch),
                   callexpr, callexpr->Args[0].get());
}




void TransferChecker::HandleCallParams(Parser_Struct* parser_struct, ExprAST *callexpr,std::string callee) {

    if (!dynamic_cast<ArgsBasedExpr*>(callexpr))
        return;

    ArgsBasedExpr *argsebasedexpr = dynamic_cast<ArgsBasedExpr*>(callexpr);
    std::string fn_name = parser_struct->base_name;
    std::string base_callee = argsebasedexpr->BaseCallee;
    if (in_vec(callee,native_methods))
        return; // skip llvm defined



    CallArgsTy &CArgs = argsebasedexpr->CArgs;

    std::vector<std::string> argnames;
    bool has_borrow = false;

    std::unordered_map<int, ExprBranch> arg_to_caller_memid;
    std::vector<std::tuple<int,int>> partialtakes;




    if (auto *callstmt = dynamic_cast<NameableCall*>(callexpr)) {
        if (callstmt->Depth>1) {
            arg_to_caller_memid[OM_self_obj] = GetLifetime(parser_struct,
                            callexpr, memid_to_branch);
        }
    }
    if (auto *newstmt = dynamic_cast<NewExprAST*>(callexpr)) {
        arg_to_caller_memid[OM_self_obj] = newstmt->GetMemId()[0];
    }




    int i=0,j=0;
    for (auto &argexpr : argsebasedexpr->Args) {
        i++;
        j++;

        if (fn_argnames.count(callee)==0)
            return;
        std::string argname = fn_argnames[callee][i-1];
        if (argname=="scope_struct")
            argname = fn_argnames[callee][i++];
        argnames.push_back(argname);


        if (!argexpr->IsOwnableExpr())
            continue;

        if (auto *nameableexpr = dynamic_cast<Nameable*>(argexpr.get()))
            if (nameableexpr->Depth!=1) 
                continue;
        if (argexpr->GetMemId()[0]==OM_non_obj)
            continue;



        ExprBranch parent_memid = GetLifetime(parser_struct,
                argexpr.get(), memid_to_branch);


        int arg_memid = fn_arg_memid[base_callee][argname];
        arg_to_caller_memid[arg_memid] = parent_memid;

    }


    i=0;
    j=0;
    for (auto &argexpr : argsebasedexpr->Args) {
        i++;
        j++;


        if (fn_argnames.count(callee)==0)
            return;
        std::string argname = fn_argnames[callee][i-1];
        if (argname=="scope_struct")
            argname = fn_argnames[callee][i++];

        if (!argexpr->IsOwnableExpr())
            continue;

        int arg_memid = fn_arg_memid[base_callee][argname];

        bool is_transfer = fn_borrows[base_callee].count(arg_memid)>0; 

        if (auto *arg_stmt = dynamic_cast<Nameable*>(argexpr.get())) {
            bool isCall = (dynamic_cast<NameableCall*>(arg_stmt) != nullptr);
            bool isIdx = (dynamic_cast<NameableIdx*>(arg_stmt) != nullptr);

            bool is_attr = (arg_stmt->Depth>1&&!isCall&&!isIdx)||isIdx;
            if (is_attr&&is_transfer&&argexpr->GetIsOwned()!=OM_non_obj) {
                LogError(callexpr->Line, "Trying to transfer ownership of a transfered object");
            }

            if (arg_stmt->Depth!=1)
                continue;
        }



        int memid = argexpr->GetMemId()[0];
        if (memid==OM_non_obj)
            continue;


        if (in_vec(arg_memid, fn_borrow_views[base_callee])) {
            fn_borrow_views[parser_struct->base_name].push_back(memid);
        }




        if (is_transfer) {
            // Route param parent to arg parent
            std::vector<ExprBranch> caller_parents;
            for (auto &borrow : fn_borrows[base_callee][arg_memid]) {
                int owner_memid = borrow.memid;
                if (arg_to_caller_memid.count(owner_memid)) {
                    caller_parents.push_back(
                        arg_to_caller_memid[owner_memid]
                    );
                }
            }

            GetCallMostRestrictive(parser_struct,
                        base_callee,
                        callexpr, argexpr.get(),
                        caller_parents,
                        arg_memid, callexpr->BranchId,
                        memid, partialtakes);
            borrow_c[memid]++;

            Data_Tree &dt = CArgs.dts[j-1];
            int ownid = argexpr->GetIsOwned();

            bool is_owned = ownid!=OM_non_obj||dt.is_borrow||dt.is_own;
            if (is_owned) {
                has_borrow=true;
                dt.is_own=false;
                dt.is_borrow=true;
                CArgs.borrows.push_back({
                        dt, argname, arg_memid, memid
                    });
            }
        }
    }


    if (has_borrow) {
        CArgs.args = argnames;
        CArgs.template_ret = fn_ret_dt[callee];
        CArgs.partialtakes = partialtakes;
        bool found = false;

        callee = GetFnVersion(parser_struct, base_callee, CArgs, found, true, true);
        if (!found) {
            Template_FnAST[base_callee][CArgs] = TheJIT->fn_map[base_callee];
            callee = GenTemplate(parser_struct, base_callee, CArgs, found);
        }
        argsebasedexpr->Callee = callee;
        argsebasedexpr->partialtakes = partialtakes;
    }
}



void TransferChecker::HandleNew(Parser_Struct* parser_struct, NewExprAST *newexpr,
        std::unordered_map<std::string, int> &seen) {
    std::string callee = newexpr->Callee;
    std::string base_callee = newexpr->BaseCallee;
    BorrowChecker(base_callee, callee, seen);

    HandleCallParams(parser_struct,newexpr, callee);
}
void TransferChecker::HandleCall(Parser_Struct* parser_struct, NameableCall *callexpr,
        std::unordered_map<std::string, int> &seen) {


    std::string callee = callexpr->Callee;
    std::string base_callee = callexpr->BaseCallee;

    if (callee=="array_append") {
        HandleAppend(parser_struct, callexpr, callee);
        return;
    }

    BorrowChecker(base_callee, callee, seen);

    HandleCallParams(parser_struct,
            callexpr, callee);
}




void TransferChecker::HandleNewVec(Parser_Struct* parser_struct,
        NewVecExprAST *newvecexpr, std::unordered_map<std::string, int> &seen) {
    int memid = newvecexpr->Memids[0];
    fn_memid_to_lastseen[parser_struct->base_name][memid] = newvecexpr;


    bool is_vec_view = in_vec(memid,fn_borrow_views[parser_struct->base_name]);

    bool is_parent_owned=false;
    ExprAST *parent_expr = nullptr;
    if (borrow_ids.count(memid)>0) {
        int parent = borrow_ids[memid][0].memid;
        if (fn_memid_to_expr[parser_struct->base_name].count(parent)) {
            parent_expr = fn_memid_to_expr[parser_struct->base_name][parent];
            parent_expr->GetDataTree().Print();

            int ownedid = parent_expr->GetIsOwned();
            is_parent_owned = parent_expr->GetIsOwned()!=OM_non_obj;


        }
    }


    for(auto &val : newvecexpr->Values) {
        int val_memid = val->GetMemId()[0];
        if (val_memid==OM_non_obj)
            return;

        if (parent_expr&&!is_vec_view) {
            RegisterMove(parser_struct,
                GetLifetime(parser_struct, parent_expr,
                    val.get(), memid_to_branch),
                newvecexpr, val.get());
        }
    }
}




void TransferChecker::HandleBinOp(Parser_Struct* parser_struct, BinaryExprAST *binop,
        std::unordered_map<std::string, int> &seen) {
    if (binop->IsCall) {
        std::string base_callee = binop->Operation;
        std::string callee = binop->Operation; // todo, specialize in generic
        BorrowChecker(base_callee, callee, seen);
        int memid = binop->Memids[0];
        memid_to_branch[memid] = FormatLifetime(binop->BranchId, memid);
    }

    if (binop->Op!='='&&!binop->is_store_sugar)
        return;

    int rmemid = binop->RHS->GetMemId()[0];
    if (rmemid==OM_non_obj)
        return;

    if (auto *lstmt = dynamic_cast<Nameable*>(binop->LHS.get())) {
        if (lstmt->Depth==1) {
            fn_memid[parser_struct->base_name][lstmt->GetName()] = rmemid;
        } else {
            if (begins_with(parser_struct->base_name,"gpu_tens")) {
            }

            RegisterMove(parser_struct,
                       GetLifetime(parser_struct, lstmt,
                            binop->RHS.get(), memid_to_branch),
                       lstmt, binop->RHS.get(), false);
        }
    }

}

void TransferChecker::HandleNameable(Parser_Struct* parser_struct, ExprAST *expr, Nameable *nameable,
        std::unordered_map<std::string, int> &seen) {

    int memid = nameable->GetMemId()[0];
    if (!nameable->IsAttr&&memid!=OM_non_obj) {
        ExprBranch branch = nameable->BranchId;
        branch.memid = memid;
        fn_memid_to_lastseen[parser_struct->base_name][memid] = expr;

        if (borrow_ids.count(memid)!=0) {
            for(auto &parent_branch : borrow_ids[memid])
              fn_memid_to_lastseen[parser_struct->base_name]\
                  [parent_branch.memid] = expr;
        }
    }

}


void TransferChecker::HandleData(Parser_Struct* parser_struct, DataExprAST *dataexpr,
        std::unordered_map<std::string, int> &seen) {
    dataexpr->SetMemId(memid_to_branch);
}


void TransferChecker::HandleUnkvar(Parser_Struct* parser_struct,
        UnkVarExprAST *unkvarexpr,
        std::unordered_map<std::string, int> &seen) {

    int i=0;
    for(auto &[name, _] : unkvarexpr->VarNames) {
        int memid = unkvarexpr->Memids[i++];
        if (memid==OM_non_obj)
            continue;
        memid_to_branch[memid] = FormatLifetime(unkvarexpr->BranchId, memid);
    }
}






void TransferChecker::HandleReturn(Parser_Struct* parser_struct, RetExprAST *ret_expr,
        std::unordered_map<std::string, int> &seen) {
    // for (auto &var : ret_expr->Vars)
    //     int memid = var->GetMemId()[0];
}

void TransferChecker::GetTransfers(Parser_Struct *parser_struct, ExprAST *expr,
        std::unordered_map<std::string, int> &seen) {

    if (auto *callexpr = dynamic_cast<NameableCall*>(expr)) {
        HandleCall(parser_struct, callexpr, seen);
        return;
    }

    if (auto *newvecexpr = dynamic_cast<NewVecExprAST*>(expr)) {
        HandleNewVec(parser_struct, newvecexpr, seen);
        return;
    }

    if (auto *binop = dynamic_cast<BinaryExprAST*>(expr)) {
        HandleBinOp(parser_struct, binop, seen);
        return;
    }

    if (auto *newexpr = dynamic_cast<NewExprAST*>(expr)) {
        HandleNew(parser_struct, newexpr, seen);
        return;
    }

    if (auto *nameable = dynamic_cast<Nameable*>(expr)) {
        HandleNameable(parser_struct, expr, nameable, seen);
        return;
    }

    if (auto *dataexpr = dynamic_cast<DataExprAST*>(expr)) {
        HandleData(parser_struct, dataexpr, seen);
        return;
    }
    if (auto *unkvarexpr = dynamic_cast<UnkVarExprAST*>(expr)) {
        HandleUnkvar(parser_struct, unkvarexpr, seen);
        return;
    }



    if (auto *ret_expr = dynamic_cast<RetExprAST*>(expr)) {
        HandleReturn(parser_struct, ret_expr, seen);
        return;
    }
}



void BorrowChecker(std::string base_callee, std::string fn_name,
                    std::unordered_map<std::string, int> &seen) {
    if (in_vec(base_callee, native_fn)
            ||fn_borrows.count(fn_name)>0
            ||seen.count(fn_name)>0
            ||!TheJIT->fn_map.count(base_callee))
        return; // skip llvm fn


    seen[fn_name] = 1;
    FunctionAST *fn_ast = TheJIT->fn_map[base_callee];
    std::vector<std::unique_ptr<ExprAST>> &Body = fn_ast->Body;
    Parser_Struct *parser_struct = fn_ast->parser_struct;
    parser_struct->function_name = fn_name;
    parser_struct->self_obj_owned = fn_with_owned_self.count(fn_name);




    *parser_struct->mem_id = parser_struct->memid_arg_offset;


    TransferChecker tc = TransferChecker(fn_name, base_callee);


    for (auto &body : Body) {
      body->Traverse([parser_struct, &tc, &seen](ExprAST *node) {
        tc.GetTransfers(parser_struct, node, seen);
      });
    }

    tc.RegisterFn();
}


void TransferChecker::RegisterFn() {
    if (fn_borrows.count(base_fn)==0) {
        fn_borrows[base_fn] = borrow_ids;
        // fn_borrow_views[base_fn] = borrow_views;
        fn_borrows_c[base_fn] = borrow_c;
        fn_bad_borrows[base_fn] = bad_borrows;

        for(auto &[memid, branch] : memid_to_branch)
            fn_memid_to_branch[base_fn][memid] = branch;

        for(auto &[memid,borrows]: borrow_branches) {
            ExprBranch take_branch = borrows[0];
            int cap = take_branch.cap;

            bool has_incomplete = borrow_c[memid]<cap;
            if (has_incomplete) {
                fn_borrows_incomplete[base_fn].push_back(memid);
            }
        }

    }
    fn_bad_borrows[fn] = bad_borrows;
}
