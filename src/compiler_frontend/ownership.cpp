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
#include "include.h"
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







std::vector<int> ExprAST::GetMemId() {
    return {-2};
}
std::vector<int> NewExprAST::GetMemId() {
    return {MemId};
}
std::vector<int> UnaryExprAST::GetMemId() {
    return Operand->GetMemId();
}
std::vector<int> BinaryExprAST::GetMemId() {
    // return LHS->GetMemId() || RHS->GetMemId();
    if (Memids.size()>0&&Memids[0]!=-2) {
        return Memids;
    }
    int id = RHS->GetMemId()[0];
    if (id>=0) return {id};
    id = LHS->GetMemId()[0];
    if (id>=0) return {id};
    return {-2};
}




// std::vector<int> GatherMemids(Nameable *expr, std::string fn,
//                                 std::string base_fn) {

//     std::vector<int> memids;
//     int cstmt = (expr->BranchId>>32)&MASK_16;
//     std::string name = expr->Name;

//     std::unordered_map<int, int> memids_to_filter = fn_cond_memid[fn][name];

//     std::cout << "In CSTMT: " << cstmt << "\n";


//     // while (true) {

//     if (fn_conditional_stmt[fn].count(cstmt)>0) {
//         ExprAST *stmt = fn_conditional_stmt[fn][cstmt];

//         int br=0;
//         stmt->Traverse([&br](ExprAST *node) {
            
//         });
//     }

//         // if (cstmt==0)
//         //     break;
//         // if (cstmt_parents[fn].count(cstmt)>0 )
//         //     break;
//         // int parent_cstmt = cstmt_parents[fn][cstmt];
//         // // std::cout << "GOT PARENT " << parent_cstmt << ", " << cstmt << "\n";
//     // }


//     for(auto &memid : memids) {
//         std::cout << "RECOVER " << memid << "\n";
//     }


//     return memids;
// }


std::vector<int> Nameable::GetMemId() {
    // Check Depth 1 owned for borrowed

    if (Memids[0] != -2) {
        return Memids;
    }
    if (Depth>1)
        return {-2};

    std::string scope = parser_struct->base_name;



    // if(fn_cond_memid[scope].count(Name)==0)
    //     return {-2};

    if (Name=="b") {
        std::unordered_map<int, int> memids_to_filter = fn_cond_memid[scope][Name];
        for(auto &[br, id] : memids_to_filter) {
            std::cout << " " << scope << "|" << Name << "  HAS  " << id << " in branch: " << (br>>16) << "\n";
        }
    }
    

    if (fn_memid[scope].count(Name)==0)
        return {-2};
    // std::cout << "for " << Name << "\n";
    return {fn_memid[scope][Name]};
}


std::vector<int> NameableCall::GetMemId() {
    // std::cout << "as call " << MemId << "\n";
    return {MemId};
}



bool Nameable::GetIsView() {
    bool is_view=false;
    if (in_vec(GetMemId()[0], fn_views[parser_struct->function_name])) {
        is_view=true;
    }
    std::string scope = (Depth==1) ? parser_struct->function_name
                                   : Inner->GetDataTree().Type;
    if (in_vec(Name, fn_class_views[scope])) {
        is_view=true;
    }
    if (Depth>1)
        return is_view||Inner->GetIsView();
    return is_view;
}

bool NameableIdx::GetIsView() {
    return Inner->GetIsView();
}
bool NameableCall::GetIsView() {
    return Inner->GetIsView();
}



uint64_t FormatLifetime(uint64_t branch, int memid) {
    return (branch&~MASK_16) | (uint16_t)memid;
}
uint64_t GetLifetime(Parser_Struct *parser_struct,
        Nameable *borrowed, std::unordered_map<int,uint64_t> &memid_to_branch) {
    if (borrowed->Depth!=1)
        return GetLifetime(parser_struct, borrowed->Inner.get(),
                           memid_to_branch);
    int memid = fn_memid[parser_struct->base_name][borrowed->Name];

    uint64_t lifetime = (memid_to_branch.count(memid)>0)
        ? memid_to_branch[memid]
        : memid; // is function arg

    uint64_t cap = (borrowed->BranchId>>16) & MASK_16;
    return (lifetime & ~(MASK_16<<16)) | (cap<<16);
}
uint64_t GetLifetime(Parser_Struct *parser_struct,
        Nameable *owner_expr, ExprAST *borrowed,
        std::unordered_map<int,uint64_t> &memid_to_branch) {
    if (owner_expr->Depth!=1)
        return GetLifetime(parser_struct, owner_expr->Inner.get(),
                           borrowed, memid_to_branch);
    int memid = fn_memid[parser_struct->base_name][owner_expr->Name];


    uint64_t lifetime = (memid_to_branch.count(memid)>0)
        ? memid_to_branch[memid]
        : memid; // is function arg

    uint64_t cap = (borrowed->BranchId>>16) & MASK_16;
    return (lifetime & ~(MASK_16<<16)) | (cap<<16);
}








void OwnedHolder::RegisterOwned(Data_Tree dt, int memid, Value *v) {
    OwnedToClear.push_back({dt, memid, v});
}


void Disown(Value *scope_struct, Data_Tree &dt, Value *ptr) {
    std::string disown_method = dt.Type+"_disown";
    if (fn_ret_dt.count(disown_method)>0) {
        set_scope_obj(scope_struct, ptr);
        call(disown_method, {scope_struct});
    }
}

void OwnedHolder::ClearOwned(Value *scope_struct, std::string fn,
                             std::string base_fn) {
    if (OwnedToClear.size()==0)
        return;
    // std::cout << "\n\n------\n";
    // if (dynamic_cast<BinaryExprAST*>(this))
    //     std::cout << "BinOp CLEAR OWNED " << "\n";
    // if (auto *stmt = dynamic_cast<NameableCall*>(this)) {
    //     std::cout << "CALL CLEAR OWNED " << "\n";
    //     p2t("clear owned " + stmt->Callee);
    // }
    // if (dynamic_cast<IfExprAST*>(this))
    //     std::cout << "IF CLEAR OWNED " << "\n";
    // if (dynamic_cast<ForExprAST*>(this))
    //     std::cout << "FOR CLEAR OWNED " << "\n";
    // if (dynamic_cast<WhileExprAST*>(this))
    //     std::cout << "WHILE CLEAR OWNED " << "\n";

    Value *previous_obj = get_scope_obj(scope_struct);
    for(auto &[dt, memid, ptr] : OwnedToClear) {
        // std::cout << "TEST " << memid << " | " << fn_borrows[base_fn].count(memid) << "\n";
        OwnedsCleared.push_back(ptr);


        bool is_conditional = ctakens.count(fn)>0&&ctakens[fn].count(memid)>0; 
        BasicBlock *AfterBB, *DisownBB;
        if (is_conditional) {
            p2t("------------------------->conditional " + std::to_string(memid));
            Value *v = Builder->CreateLoad(boolTy, ctakens[fn][memid]);
            call("print_bool", {v}); 
            Function *TheFunction = Builder->GetInsertBlock()->getParent();
            AfterBB  = BasicBlock::Create(*TheContext, "disown.after",
                                TheFunction);
            DisownBB = BasicBlock::Create(*TheContext, "disown.disown",
                                TheFunction);

            Builder->CreateCondBr(
                        Builder->CreateLoad(boolTy, ctakens[fn][memid]),
                        AfterBB, DisownBB);
            Builder->SetInsertPoint(DisownBB);
        }
        if (!in_vec(memid, fn_views[fn])&&dt.Type=="array")
            ArrayClearOwned(scope_struct, dt, ptr);
        p2t("disown dispatch");
        // call("print_void_ptrC", {ptr});
        Disown(scope_struct, dt, ptr);
        if (is_conditional) {
            p2t("cond free: ");
            call("print_void_ptr", {ptr});
            call("free", {ptr});
            block_values[DisownBB] = function_values[fn];
            Builder->CreateBr(AfterBB);
            Builder->SetInsertPoint(AfterBB);
        }
    }
    set_scope_obj(scope_struct, previous_obj);
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



void CheckBadBorrow(Parser_Struct *parser_struct, 
                 std::unordered_map<int,std::vector<uint64_t>> &borrow_ids,
                 std::unordered_map<int,std::vector<uint64_t>> &borrow_branches,
                 std::unordered_map<int,int> &borrow_c,
                 std::vector<std::tuple<int,int,int>> &bad_borrows,
                 std::unordered_map<int,uint64_t> &memid_to_branch,
                 Nameable *parent, ExprAST *nameable
            ) {
    int memid = nameable->GetMemId()[0];
    if (memid==-2)
        return;
    // std::cout << "check  " << parser_struct->function_name << " | " << memid << "\n";

    if (!dynamic_cast<Nameable*>(nameable))
        return;

    uint64_t taken_lifetime = GetLifetime(parser_struct,
        (Nameable*)nameable, memid_to_branch);

    if (borrow_ids.count(memid)) {

        if (parent!=nullptr) {
            if (parent->GetIsOwned()!=-2&&nameable->GetIsOwned()==-2) {
                LogErrorS(nameable->Line, "Cannot assign a GC arena object to an owned."+ parser_struct->function_name + "|" + nameable->Name);
            }
        }


        uint64_t expr_branch = nameable->BranchId;
        int expr_control_stmt = (expr_branch>>32)&MASK_16;
        int expr_depth = (expr_branch>>48)&MASK_16;
        int line = nameable->Line;

        int i=0;
        uint64_t first_taken_branch = borrow_branches[memid][0];
        for (auto &lifetime : borrow_ids[memid]) {
            uint64_t taken_branch = borrow_branches[memid][i++];


            bool has_loop = match_cstmt_has_loop(
                        parser_struct->base_name,
                    (taken_lifetime>>32)&MASK_16, expr_control_stmt);

            int control_stmt = (lifetime>>32)&MASK_16;
            int depth = (lifetime>>48)&MASK_16;
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
                &&((first_taken_branch>>32)&MASK_16) != ((taken_branch>>32)&MASK_16)) {
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






void DataExprAST::SetMemId(std::unordered_map<int,uint64_t> &memid_to_branch) {
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


inline void RegisterBorrow(Parser_Struct *parser_struct,
             uint64_t parent_lifetime,
             NameableCall *callexpr,
             std::unique_ptr<ExprAST> &expr,
             std::unordered_map<int,std::vector<uint64_t>> &borrow_ids,
             std::unordered_map<int,std::vector<uint64_t>> &borrow_branches,
             std::unordered_map<int,int> &borrow_c,
             std::unordered_map<int,uint64_t> &memid_to_branch,
             std::vector<std::tuple<int,int,int>> &bad_borrows
         ) {
    if (callexpr->GetIsView())
        return;

    int appended_memid = expr->GetMemId()[0];
    if (appended_memid == -2)
        return;
    uint64_t branch = expr->BranchId;

    // std::cout << "==========REGISTER " << parser_struct->base_name << " | " << appended_memid << " in branch " << ((branch>>32)&MASK_16) << "\n"; 

    borrow_ids[appended_memid].push_back(parent_lifetime);
    borrow_branches[appended_memid].push_back(branch);
    borrow_c[appended_memid]++;

    CheckBadBorrow(parser_struct, borrow_ids, borrow_branches, borrow_c,
                   bad_borrows, memid_to_branch, callexpr->Inner.get(), expr.get());
}


void GetCallMostRestrictive(Parser_Struct *parser_struct,
            std::string callee,
            NameableCall *callexpr,
            ExprAST *argexpr,
            std::vector<uint64_t> &arg_parents,
            int arg_memid, uint64_t callexpr_branch,
            int memid,
            std::unordered_map<int, std::vector<uint64_t>> &borrow_ids,
            std::unordered_map<int,int> &borrow_c,
            std::vector<std::tuple<int,int,int>> &bad_borrows,
            std::unordered_map<int,std::vector<uint64_t>> &borrow_branches,
            std::unordered_map<int,uint64_t> &memid_to_branch,
            std::vector<std::tuple<int,int>> &partialtakes) {
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
    if (has_incomplete) {
        fn_borrows_incomplete[parser_struct->base_name].push_back(memid);
        partialtakes.push_back({memid,arg_memid});
    }


    uint64_t first_branch = arg_parents[0];
    int cstmt = (first_branch>>32) & MASK_16;
    borrow_ids[memid].push_back(first_branch);
    borrow_branches[memid].push_back(callexpr_branch);
    int most_restrictive = 0;
    for (int i=1; i<arg_parents.size(); ++i) {
        uint64_t branch = arg_parents[i];

        borrow_ids[memid].push_back(branch);
        borrow_branches[memid].push_back(callexpr_branch);

        int cstmt_i = (branch>>32) & MASK_16;
        if (cstmt_i>cstmt)
            most_restrictive=i;
    }
    // std::cout << "most restrictive " << most_restrictive << " | " << arg_parents.size()<< "\n";

    CheckBadBorrow(parser_struct, borrow_ids, borrow_branches, borrow_c,
                   bad_borrows, memid_to_branch, nullptr, argexpr);

    return;
}

void RegisterCallBorrow(Parser_Struct *parser_struct,
            NameableCall *callexpr, std::string callee,
            std::unordered_map<int,std::vector<uint64_t>> &borrow_ids,
            std::unordered_map<int,std::vector<uint64_t>> &borrow_branches,
            std::unordered_map<int,int> &borrow_c,
            std::vector<std::tuple<int,int,int>> &bad_borrows,
            std::unordered_map<int,uint64_t> &memid_to_branch,
            std::unordered_map<std::string, int> &seen) {
    std::string fn_name = parser_struct->base_name;
    std::string base_callee = callexpr->BaseCallee;
    // if (fn_argnames.count(callee))
        // std::cout << "Failed for " << callee << "\n";
    if (in_vec(callee,native_methods))
        return; // skip llvm defined


    CallArgsTy &CArgs = callexpr->CArgs;

    std::vector<std::string> argnames;
    bool has_borrow = false;

    std::unordered_map<int, uint64_t> arg_to_caller_memid;
    std::vector<std::tuple<int,int>> partialtakes;

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

            if (nameableexpr->GetMemId()[0]==-2)
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

            int memid = nameableexpr->GetMemId()[0];
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
                            argexpr.get(),
                            caller_parents,
                            arg_memid, callexpr->BranchId,
                            memid, borrow_ids, borrow_c,
                            bad_borrows, borrow_branches,
                            memid_to_branch, partialtakes);
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
        CArgs.partialtakes = partialtakes;
        bool found = false;

        callee = GetFnVersion(parser_struct, base_callee, CArgs, found, true, true);
        if (!found) {
            Template_FnAST[base_callee][CArgs] = TheJIT->fn_map[base_callee];
            callee = GenTemplate(parser_struct, base_callee, CArgs, found);
        }
        callexpr->Callee = callee;
        callexpr->partialtakes = partialtakes;
    }
}








void GetBorrows(Parser_Struct *parser_struct,
                std::unordered_map<std::string, int> &seen,
                ExprAST *expr,
                std::unordered_map<int,std::vector<uint64_t>> &borrow_ids,
                std::unordered_map<int,std::vector<uint64_t>> &borrow_branches,
                std::unordered_map<int,int> &borrow_c,
                std::vector<std::tuple<int,int,int>> &bad_borrows,
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
                           callexpr, callexpr->Args[0],
                           borrow_ids, borrow_branches, borrow_c,
                           memid_to_branch, bad_borrows);
            return;
        }
        RegisterCallBorrow(parser_struct,
                callexpr, callee, borrow_ids, borrow_branches, borrow_c,
                bad_borrows, memid_to_branch, seen);

        return;
    }


    if (auto *binop = dynamic_cast<BinaryExprAST*>(expr)) {
        if (binop->IsCall) {
            std::string base_callee = binop->Operation;
            std::string callee = binop->Operation; // todo, specialize in generic
            BorrowChecker(base_callee, callee, seen);

        }

        if (binop->Op!='='&&!binop->is_store_sugar)
            return;

        int rmemid = binop->RHS->GetMemId()[0];
        if (rmemid==-2)
            return;

        if (auto *lstmt = dynamic_cast<Nameable*>(binop->LHS.get())) {
            if (lstmt->Depth==1) {
                fn_memid[parser_struct->base_name][lstmt->GetName()] = rmemid;
            }
        }

        return;
    }


    if (auto *nameable = dynamic_cast<Nameable*>(expr)) {
        int memid = nameable->GetMemId()[0];
        if (!nameable->IsAttr&&memid!=-2) {
            uint64_t branch = nameable->BranchId;
            branch = (branch&~MASK_16) | memid;
            // std::cout << "Last Seen " << nameable->GetName() << ": " << memid << " | " << expr << "\n";

            fn_memid_to_lastseen[parser_struct->base_name][memid] = expr;

            if (borrow_ids.count(memid)!=0) {
                for(auto &parent_branch : borrow_ids[memid])
                  fn_memid_to_lastseen[parser_struct->base_name]\
                      [parent_branch&MASK_16] = expr;
            }
        }
        return;
    }

    if (auto *dataexpr = dynamic_cast<DataExprAST*>(expr)) {
        dataexpr->SetMemId(memid_to_branch);
        return;
    }
    if (auto *unkvarexpr = dynamic_cast<UnkVarExprAST*>(expr)) {
        int i=0;
        for(auto &[name, _] : unkvarexpr->VarNames) {
            int memid = unkvarexpr->Memids[i++];
            if (memid==-2)
                continue;
            memid_to_branch[memid] = FormatLifetime(unkvarexpr->BranchId, memid);
        }
        return;
    }



    if (auto *ret_expr = dynamic_cast<RetExprAST*>(expr)) {
        for (auto &var : ret_expr->Vars) {
            retcount++;

            int memid = var->GetMemId()[0];
            if (memid>=-1) {
                retids.push_back(memid);
            }
        }
    }
}





void BorrowChecker(std::string base_callee, std::string fn_name,
                    std::unordered_map<std::string, int> &seen) {
    // fn_name = base_callee;
    // std::cout << "BorrowChecker " << base_callee << "|" << fn_name << "\n";
    if (in_vec(base_callee, native_fn)
            ||fn_borrows.count(fn_name)>0
            ||seen.count(fn_name)>0
            ||!TheJIT->fn_map.count(base_callee))
        return; // skip llvm fn
    // std::cout << "BorrowChecker 2:" << base_callee << "|" << fn_name << "\n";

    seen[fn_name] = 1;
    FunctionAST *fn_ast = TheJIT->fn_map[base_callee];
    std::vector<std::unique_ptr<ExprAST>> &Body = fn_ast->Body;
    Parser_Struct *parser_struct = fn_ast->parser_struct;
    std::unordered_map<int,std::vector<uint64_t>> borrow_ids, borrow_branches;
    std::unordered_map<int,int> borrow_c;
    std::unordered_map<int,uint64_t> memid_to_branch;
    std::vector<std::tuple<int,int,int>> bad_borrows;
    std::vector<int> retids;
    int retcount=0;
    parser_struct->function_name = fn_name;


    *parser_struct->mem_id = parser_struct->memid_arg_offset;

    for (auto &body : Body) {
      body->Traverse([parser_struct, &seen,
              &borrow_ids, &borrow_branches, &borrow_c, &bad_borrows, &memid_to_branch,
              &retids, &retcount](ExprAST *node) {

        GetBorrows(parser_struct, seen, node,
                    borrow_ids, borrow_branches,
                    borrow_c,
                    bad_borrows,
                    memid_to_branch,
                    retids, retcount
                );
      });
    }

    if (fn_borrows.count(base_callee)==0) {
        fn_borrows[base_callee] = borrow_ids;
        fn_borrows_c[base_callee] = borrow_c;
        fn_bad_borrows[base_callee] = bad_borrows;
        fn_rets[base_callee] = retids;
        fn_retscount[base_callee] = retcount;
        for(auto &[memid, branch] : memid_to_branch)
            fn_memid_to_branch[base_callee][memid] = branch;
        for(auto &[memid,borrows]: borrow_ids) {
            uint64_t fn_owner_branch = borrows[0];
            int cap = (fn_owner_branch>>16)&MASK_16;

            bool has_incomplete = borrow_c[memid]<cap;
            if (has_incomplete) {
                fn_borrows_incomplete[base_callee].push_back(memid);
            }

        }
    }
    fn_bad_borrows[fn_name] = bad_borrows;



    // int i=0;
    // for (auto &version : FnVersion[fn_name]) {
    //     if(i==0) {
    //         i++;
    //         continue;
    //     }
    //     fn_borrows_incomplete[fn_name+"_"+std::to_string(i++)] = fn_borrows_incomplete[fn_name];
    // }
}
