

#include "expressions.h"
#include "llvm/IR/Value.h"


#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <execution>
#include <execinfo.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "include.h"
#include "../runtime/compiler_frontend/parser_struct.h"
#include "../runtime/data_types/data_tree.h"
#include "logging.h"
#include "modules.h"

#define MASK_16 0xFFFFULL


using namespace llvm;
namespace fs = std::filesystem;



std::vector<std::string> imported_libs,fn_called;
std::map<std::string, std::vector<std::string>> lib_submodules;

std::unordered_map<std::string, std::vector<CallArgsTy>> FnVersion;
std::unordered_map<std::string, std::vector<std::tuple<std::string, std::string, Data_Tree>>> FnDynArgs;
std::unordered_map<std::string,int> FnLastVersion;
std::unordered_map<std::string,std::unordered_map<int,int>> cstmt_parents;
std::unordered_map<std::string, std::vector<CallArgsTy>> FnTemplates;

std::unordered_map<std::string, std::unordered_map<std::string, int>> function_owns, fn_memid, fn_arg_memid;
std::unordered_map<std::string, std::vector<int>> function_escapes, function_callee_escapes;
std::unordered_map<std::string,
       std::unordered_map<int,int>> fn_borrows_c;
std::unordered_map<std::string,
       std::vector<int>> fn_bad_borrows;
std::unordered_map<std::string, std::unordered_map<int, std::vector<uint64_t>>> fn_borrows;
std::unordered_map<std::string,int> function_own_ret_count, fn_retscount, fn_owns;

std::unordered_map<std::string,
       std::vector<int>> fn_borrows_incomplete;


std::unordered_map<std::string,std::vector<int>> fn_rets;

std::vector<std::tuple<FunctionAST *,
       std::unique_ptr<PrototypeAST>,
       Parser_Struct*,
       std::string, std::string>> generics_fn;


void bt(int cut) {
    void* callstack[128];
    int frames = backtrace(callstack, 128);
    char** strs = backtrace_symbols(callstack, frames);

    int cut_at = std::min(frames, cut);
    
    if (strs != nullptr) {
        for (int i = 0; i < cut_at; ++i) {
            std::cout << strs[i] << "\n\n";
        }
        free(strs);
    }
}


//===----------------------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------------------===//
void ExprAST::Checks() {}
void ExprAST::SetType(std::string Type) {
  this->Type=Type;
  this->ReturnType=Type;
}
Data_Tree ExprAST::GetDataTree(bool from_assignment) {
  return Data_Tree(this->Type);
}
void ExprAST::SetReturnType(std::string ReturnType) {
  this->ReturnType=ReturnType;
}

void ExprAST::SetIsVarLoad(bool isVarLoad) {
  this->isVarLoad=isVarLoad;
}
bool ExprAST::GetIsVarLoad() {
  return isVarLoad;
}

bool ExprAST::GetNameSolveToLast() {
  return NameSolveToLast;
}
void ExprAST::SetNameSolveToLast(bool NameSolveToLast) {
  this->NameSolveToLast=NameSolveToLast;
}

void ExprAST::SetSelf(bool Self) {
  this->isSelf=Self;
}
bool ExprAST::GetSelf() {
  return isSelf;
}

void ExprAST::SetSolverIncludeScope(bool SolverIncludeScope) {
  this->SolverIncludeScope=SolverIncludeScope;
}
bool ExprAST::GetSolverIncludeScope() {
  return SolverIncludeScope;
}

void ExprAST::SetIsAttribute(bool Attribute) {
  this->isAttribute=Attribute;
}
bool ExprAST::GetIsAttribute() {
  return isAttribute;
}


void ExprAST::SetMemId() {
}
void ExprAST::SetMemId(std::unordered_map<int, uint64_t>&) {
}




void ExprAST::SetIsMsg(bool isMessage) {
  this->isMessage=isMessage;
}
bool ExprAST::GetIsMsg() {
  return isMessage;
}


void ExprAST::SetPreDot(std::string pre_dot) {
  this->_pre_dot=pre_dot;
}
std::string ExprAST::GetPreDot() {
  return _pre_dot;
}

std::string ExprAST::GetName() {
  return Name;
}
void ExprAST::SetName(std::string Name) {
  this->Name=Name;
}


void ExprAST::SetIsVec(bool isVec) {
  this->isVec=isVec;
}
bool ExprAST::GetIsVec() {
  return isVec;
}


void ExprAST::SetIsList(bool isList) {
  this->isList=isList;
}
bool ExprAST::GetIsList() {
  return isList;
}



void ExprTieBranch(Parser_Struct *parser_struct,
        std::vector<std::unique_ptr<ExprAST>> &Body,
        int branchid) {
    for (auto &body : Body) {
        int cstmtid = (body->BranchId >> 32)&MASK_16;
        if (cstmtid!=branchid) {
            cstmt_parents[parser_struct->function_name][cstmtid] = branchid;
        }
    }
}

void ExprSetBranch(ExprAST *expr, uint64_t branch_id) {
    if (expr->BranchId<=2)
        expr->BranchId = branch_id;
    else {
        uint64_t branch = expr->BranchId & MASK_16;
        if (branch<=2) {
            expr->BranchId = (expr->BranchId &~MASK_16) | ( branch_id& MASK_16);
        }
    }
}


bool ExprAST::GetNeedGCSafePoint() {
    return false;
}
bool BinaryExprAST::GetNeedGCSafePoint() {
    return (LHS->GetNeedGCSafePoint()||RHS->GetNeedGCSafePoint());
}


bool MatchBorrows(Parser_Struct *parser_struct,
        std::string Callee, CallArgsTy &CArgs) {
    if (fn_arg_memid.count(Callee)==0)
        return false;

    bool has_borrow = false;
    std::vector<std::string> argnames;
    
    int i=0;
    for (auto &argname : fn_argnames[Callee]) {
        if (argname=="scope_struct")
            continue;
        argnames.push_back(argname);
        if (i>=CArgs.dts.size()) // todo: this break is skipping default args
            break;

        Data_Tree &dt = CArgs.dts[i++];
        if (!dt.is_own&&!dt.is_borrow)
            continue;

        int arg_memid = fn_arg_memid[Callee][argname];
        if (fn_borrows[Callee].count(arg_memid)) {
            
            auto &borrow_branches = fn_borrows[Callee][arg_memid];
            uint64_t first_borrow = borrow_branches[0];
            for (int j=1; j<borrow_branches.size(); j++) {
                if (first_borrow!=borrow_branches[j])
                    LogErrorS(parser_struct->line, "The code may try to borrow a value in non mutually exclusive branches.");
            }

            dt.is_own=0;
            dt.is_borrow=true;
            has_borrow = true;
            CArgs.borrows.push_back({
                    dt, argname, arg_memid
                });
        }
    }
 
    if (has_borrow) {
        CArgs.args = argnames;
        CArgs.template_ret = fn_ret_dt[Callee];
        CArgs.template_ret.Print();
    }
    

    return has_borrow;
}


std::string SolveTemplate(Parser_Struct *parser_struct, std::string Callee, CallArgsTy &CArgs) {

  // bool has_borrow = MatchBorrows(parser_struct, Callee, CArgs);
  bool has_borrow = false;

  bool found = true;
  Callee = GetFnVersion(parser_struct, Callee, CArgs, found, true, true);

  if (!found) {
      if (Template_FnAST.count(Callee)>0) {
        Callee = GenTemplate(parser_struct, Callee, CArgs, found);
      }
      else if (has_borrow) {
        Template_FnAST[Callee][CArgs] = TheJIT->fn_map[Callee];
        Callee = GenTemplate(parser_struct, Callee, CArgs, found);
      }

      

      if (!found) {
          if (FnLastVersion.count(Callee)==0) {
                LogErrorS(parser_struct->line, "Function " + Callee + " does not exist.");
          } else 
              FnNotFound(parser_struct, Callee, CArgs);
      }
  }
  FunctionChecks(Callee);
  return Callee;
}


void TemplateSolveCompiledArgs(std::string Callee, std::string base_callee) {
  if (Callee!=base_callee && Fn_Compiled_Args.count(base_callee)>0) {
    std::vector<std::unique_ptr<CompiledArgs>> vec_copy;
    for (auto &val : Fn_Compiled_Args[base_callee])
        vec_copy.push_back(std::make_unique<CompiledArgs>(val->dt, val->name));
    
    Fn_Compiled_Args[Callee] = std::move(vec_copy);
  }
}

// nlohmann::json ExprAST::toJSON() {
//   nlohmann::json j;
//   return j;
// }
//
//

void ExprAST::Traverse(const std::function<void(ExprAST*)>& fn) {
    fn(this);
}

void FinishExprAST::Traverse(const std::function<void(ExprAST*)>& fn) {
    fn(this);
    for (auto &expr : Bodies) 
        expr->Traverse(fn);
}

void IntervalLoopExprAST::Traverse(const std::function<void(ExprAST*)>& fn) {
    fn(this);
    Body[0]->Traverse(fn);
}

void ForExprAST::Traverse(const std::function<void(ExprAST*)>& fn) {
    fn(this);
    Start->Traverse(fn);
    End->Traverse(fn);
    Step->Traverse(fn);
    for (auto &expr : Body) 
        expr->Traverse(fn);
}

void IfExprAST::Traverse(const std::function<void(ExprAST*)>& fn) {
    fn(this);
    for (auto &expr : Then) 
        expr->Traverse(fn);
    for (auto &expr : Else) 
        expr->Traverse(fn);
}


void ForEachExprAST::Traverse(const std::function<void(ExprAST*)>& fn) {
    fn(this);
    Vec->Traverse(fn);
    for (auto &expr : Body)
        expr->Traverse(fn);
}

void WhileExprAST::Traverse(const std::function<void(ExprAST*)>& fn) {
    fn(this);
    Cond->Traverse(fn);
    for (auto &expr : Body)
        expr->Traverse(fn);
}

void AsyncExprAST::Traverse(const std::function<void(ExprAST*)>& fn) {
    fn(this);
    for (auto &expr : Body)
        expr->Traverse(fn);
}

void AsyncsExprAST::Traverse(const std::function<void(ExprAST*)>& fn) {
    fn(this);
    for (auto &expr : Body)
        expr->Traverse(fn);
}
void SpawnExprAST::Traverse(const std::function<void(ExprAST*)>& fn) {
    fn(this);
    for (auto &expr : Body)
        expr->Traverse(fn);
}

void MainExprAST::Traverse(const std::function<void(ExprAST*)>& fn) {
    fn(this);
    for (auto &expr : Bodies)
        expr->Traverse(fn);
}

void UnkVarExprAST::Traverse(const std::function<void(ExprAST*)>& fn) {
    fn(this);
    for (auto &var : VarNames)
        var.second->Traverse(fn);
}
void DataExprAST::Traverse(const std::function<void(ExprAST*)>& fn) {
    fn(this);
    for (auto &var : VarNames)
        var.second->Traverse(fn);
}
void BinaryExprAST::Traverse(const std::function<void(ExprAST*)>& fn) {
    fn(this);
    LHS->Traverse(fn);
    RHS->Traverse(fn);
}
void UnaryExprAST::Traverse(const std::function<void(ExprAST*)>& fn) {
    fn(this);
    Operand->Traverse(fn);
}
void RetExprAST::Traverse(const std::function<void(ExprAST*)>& fn) {
    fn(this);
    for (auto &var : Vars)
        var->Traverse(fn);
}

void ObjectExprAST::Traverse(const std::function<void(ExprAST*)>& fn) {
    fn(this);
    for (unsigned i = 0, e = this->VarNames.size(); i != e; ++i) {
        if (!this->HasInit[i]) { // callee init
            if (!VarNames[i].second)
                continue;
            VarNames[i].second->Traverse(fn);
        }
    }
}

void Nameable::Traverse(const std::function<void(ExprAST*)>& fn) {
    fn(this);
    if (Depth>1)
        Inner->Traverse(fn);
}

void NameableIdx::Traverse(const std::function<void(ExprAST*)>& fn) {
    fn(this);
    Inner->Traverse(fn);
}


void NameableCall::Traverse(const std::function<void(ExprAST*)>& fn) {
    fn(this);
    for (auto &var : Args)
        var->Traverse(fn);
    if (Depth>1)
        Inner->Traverse(fn);
}


 

void ExprAST::SetCValues(Parser_Struct *parser_struct) {
    if (!this->parser_struct||!parser_struct)
        return;
    this->parser_struct->function_name = parser_struct->function_name;
    this->parser_struct->cvalues = parser_struct->cvalues;
}


void ForExprAST::SetCValues(Parser_Struct *parser_struct) {
    this->parser_struct->function_name = parser_struct->function_name;
    this->parser_struct->cvalues = parser_struct->cvalues;
    for (auto &expr : Body) 
        expr->SetCValues(parser_struct);
}

void ForEachExprAST::SetCValues(Parser_Struct *parser_struct) {
    this->parser_struct->function_name = parser_struct->function_name;
    this->parser_struct->cvalues = parser_struct->cvalues;
    for (auto &expr : Body)
        expr->SetCValues(parser_struct);
}

void WhileExprAST::SetCValues(Parser_Struct *parser_struct) {
    this->parser_struct->function_name = parser_struct->function_name;
    this->parser_struct->cvalues = parser_struct->cvalues;
    for (auto &expr : Body)
        expr->SetCValues(parser_struct);
}

void AsyncExprAST::SetCValues(Parser_Struct *parser_struct) {
    this->parser_struct->function_name = parser_struct->function_name;
    this->parser_struct->cvalues = parser_struct->cvalues;
    for (auto &expr : Body)
        expr->SetCValues(parser_struct);
}

void AsyncsExprAST::SetCValues(Parser_Struct *parser_struct) {
    this->parser_struct->function_name = parser_struct->function_name;
    this->parser_struct->cvalues = parser_struct->cvalues;
    for (auto &expr : Body)
        expr->SetCValues(parser_struct);
}
void SpawnExprAST::SetCValues(Parser_Struct *parser_struct) {
    this->parser_struct->function_name = parser_struct->function_name;
    this->parser_struct->cvalues = parser_struct->cvalues;
    for (auto &expr : Body)
        expr->SetCValues(parser_struct);
}

void AsyncFnPriorExprAST::SetCValues(Parser_Struct *parser_struct) {
    this->parser_struct->function_name = parser_struct->function_name;
    this->parser_struct->cvalues = parser_struct->cvalues;
}








CallArgsTy::CallArgsTy(std::vector<Data_Tree> dts) {
    for (auto &dt : dts) {
        if (dt.Type=="Scope_Struct")
            continue;
        this->dts.push_back(dt);
    }
}


CallArgsTy::CallArgsTy(std::vector<std::unique_ptr<ExprAST>> *exprs) {
    if (!exprs)
        return;
    for (auto &expr : *exprs) {
        Data_Tree dt = expr->GetDataTree();
        if (dt.Type=="Scope_Struct")
            continue;
        this->dts.push_back(dt);
    }
}

int SetFnVersion(std::string fn, CallArgsTy CArgs, bool overwrite) {
    if (FnLastVersion.count(fn)==0||overwrite) {
        FnLastVersion[fn] = 1;
        CArgs.version = 0;
        CArgs.version_str = fn;
        if (overwrite)
            FnVersion[fn].clear();
        FnVersion[fn].push_back(CArgs);
        return 0;
    }
    FnLastVersion[fn]++;
    int id = FnLastVersion[fn];
    CArgs.version = id;
    CArgs.version_str = fn+"_"+std::to_string(id);
    FnVersion[fn].push_back(CArgs);
    return id;
}
void AddFnVersion(std::string fn, CallArgsTy CArgs, int idx) {
    if (FnLastVersion.count(fn)==0)
        LogErrorS(-1, "Can't AddFn " + fn + ". No previous version existed.");
    CArgs.version_str = (idx==0) ? fn : fn+"_"+std::to_string(idx);
    CArgs.version = idx;

    FnVersion[fn].push_back(CArgs);
}

bool CompareDTs(std::vector<Data_Tree> l, std::vector<Data_Tree> r, bool accept_layout=true, bool match_borrows=false) {
    if(l.size()!=r.size())
        return false;
    for (int i=0; i<l.size(); ++i) {
        // std::cout << "COMPARE" << "\n";
        // l[i].Print();
        // r[i].Print();
        //
        if (l[i].Type=="any"||r[i].Type=="any")
            continue;
        if (match_borrows&&l[i].is_borrow!=r[i].is_borrow)
            return false;
        if (!accept_layout&&l[i].Type=="layout")
            return false;
        if (l[i].Compare(r[i])>0)
            return false;
    }
    return true;
}



PrototypeAST::PrototypeAST(Parser_Struct *parser_struct,
              const std::string &BaseName,
              const std::string &fn,
              CallArgsTy CArgs, CallArgsTy &templ)
      : BaseName(BaseName), Name(fn), CArgs(CArgs) {
    this->parser_struct = parser_struct;

    FnVersion[BaseName].push_back(CArgs);

    int size = CArgs.args.size();
    for (int i=0; i<size; ++i) {
        std::string arg_name = CArgs.args[i];
        Data_Tree dt = templ.dts[i];
        data_typeVars[this->Name][arg_name] = dt;
        this->Args.push_back(arg_name);
        this->Types.push_back(dt);
    }
    for (auto &[_, name, dt] : CArgs.dyn_args) {
        this->Args.push_back(name);
        this->Types.push_back(dt);
        data_typeVars[this->Name][name] = dt;
    }

    ReturnType = CArgs.template_ret;

    
    fn_ret_dt[this->Name] = CArgs.template_ret;
    native_fn.push_back(this->Name);

    int ctx_offset = (parser_struct->gpu>0) ? 0 : 1;
    if (parser_struct->gpu==0) {
        this->Types.insert(this->Types.begin(), Data_Tree("Scope_Struct"));
        this->Args.insert(this->Args.begin(), "scope_struct");
    }


    int required_args = this->Args.size()-ctx_offset;
    Function_Required_Arg_Count[this->Name] = required_args; // Desconsider scope_struct
    Function_Arg_Count[this->Name] = required_args;
    fn_argnames[this->Name] = this->Args;
}



void AssignGenericTree(Parser_Struct *parser_struct,
        Data_Tree dt, Data_Tree &templ_dt,
        std::unordered_map<std::string,Data_Tree> &generics_map,
        CallArgsTy &templ,
        FnCompiledValues &cvalues) {


    // std::cout << "\n\ntree in layout" << "\n";
    // dt.Print();
    // templ_dt.Print();
    // std::cout << "generic? " << templ_dt.is_generic << "\n";

    std::string templ_type = templ_dt.Type;
    if (templ_dt.is_generic) {
        if (generics_map.count(templ_type)>0) {
            generics_map[templ_type].Print();
            if (dt.Compare(generics_map[templ_type])>0) {
                LogErrorS(-1, "Assigned 2 different values for a single generic type.");
                return;
            }
        }
        templ_dt = GenericUnmangleType(dt, templ_dt);
        generics_map[templ_type] = templ_dt;
        return;
    }
    else if (templ_type!="layout"&&\
               !in_vec(templ_type, data_tokens) && !in_vec(templ_type, compound_tokens)&&\
               ClassSize.count(templ_type)==0) {
        // like a layout nested data

        // is dynamic
        if (parser_struct->cvalues.dts.count(dt.Type)==0) {
            if(templ.dyn_args_dict.count(templ_type)>0)
                return;
            templ.dyn_args_dict[templ_type] = 1;
            templ.dyn_args.push_back(
                                    {dt.Type, templ_type,
                                    data_typeVars[parser_struct->function_name][dt.Type]}
                                );
            return;
        } else
            std::cout << "SKIP " << dt.Type << "=" << templ_type << "\n";
        std::string type = parser_struct->cvalues.dts[dt.Type].Type;
        if (type=="int")
            cvalues.AddInt(templ_type, parser_struct->cvalues.ints[dt.Type]);
    }

    for (int i=0; i<templ_dt.Nested_Data.size(); ++i) {
        if (templ_dt.Nested_Data[i].Type=="smem")
            continue;
        AssignGenericTree(parser_struct,
                          dt.Nested_Data[i], templ_dt.Nested_Data[i],
                          generics_map, templ, cvalues);
    }
}

void DeriveTypedGeneric(Data_Tree templ_dt, Data_Tree &ret_dt,
        std::unordered_map<std::string,Data_Tree> &generics_map) {

    if (generics_map.count(templ_dt.Type)>0) {
        std::cout << "SKIP " << templ_dt.Type << "\n";
        ret_dt = generics_map[templ_dt.Type].Type;
    }
    else
        ret_dt.Type = templ_dt.Type;
    

    for (int i=0; i<ret_dt.Nested_Data.size(); ++i) {
        ret_dt.Nested_Data.push_back(Data_Tree(""));
        DeriveTypedGeneric(templ_dt.Nested_Data[i], ret_dt.Nested_Data[i],
                            generics_map);
    }
}

void AssignGenericTypes(Parser_Struct *parser_struct, CallArgsTy cargs, CallArgsTy &templ,
                              FnCompiledValues &cvalues) {
    std::unordered_map<std::string,Data_Tree> generics_map;

    for (int i=0; i<templ.dts.size(); ++i)
        AssignGenericTree(parser_struct, cargs.dts[i], templ.dts[i], generics_map, templ, cvalues);

    if (templ.template_ret.HasGeneric()) {
        Data_Tree ret_dt = Data_Tree("");
        DeriveTypedGeneric(templ.template_ret, ret_dt, generics_map);
        templ.template_ret = ret_dt;
    }

    for (auto &[name, dt] : generics_map) {
        std::cout << name << "\n";
    }
}


std::vector<std::tuple<std::string, std::string, Data_Tree>> GetDynamicArgs(Parser_Struct *parser_struct, std::string fn,
                        CallArgsTy CArgs, bool &found) {
    FunctionAST *fn_ast=nullptr; 
    for (auto &tpair : Template_FnAST[fn]) {
        CallArgsTy t_templ = tpair.first;
        CallArgsTy templ = t_templ;

        if (!CompareDTs(CArgs.dts, templ.dts, true, false))
            continue;

        fn_ast = tpair.second;
        FnCompiledValues cvalues;
        AssignGenericTypes(parser_struct, CArgs, templ, cvalues);

        return templ.dyn_args;
    }

    int i=0;
    std::cout << "\nFound" << "\n";
    for (auto &tpair : Template_FnAST[fn]) {
        if (i>3) {
            std::cout << "...\n";
            break;
        }
        print_dt_vec(tpair.first.dts);
    } 
    std::cout << "\nSent" << "\n";
    print_dt_vec(CArgs.dts);
    LogErrorS(parser_struct->line, "Could not match arguments for "+fn);

}


std::string GenTemplate(Parser_Struct *parser_struct, std::string fn,
                        CallArgsTy CArgs, bool &found,
                        bool is_op) {

    FunctionAST *fn_ast=nullptr; 
    for (auto &tpair : Template_FnAST[fn]) {
        CallArgsTy t_templ = tpair.first;
        CallArgsTy templ = t_templ;
        if (!CompareDTs(CArgs.dts, templ.dts, true, true))
            continue;


        fn_ast = tpair.second;
        FnCompiledValues cvalues;
        AssignGenericTypes(parser_struct, CArgs, templ, cvalues);



        // std::cout << "assign for " << "\n";
        // print_dt_vec(CArgs.dts);
        // print_dt_vec(templ.dts);


        if (is_op)
            fn = templ.dts[0].Type + "_" + templ.dts[1].Type + "_" + fn;
        


        std::string base_name = fn;
        int idx;
        if (FnLastVersion.count(fn)==0) {
            idx = 0;
            FnLastVersion[fn] = 1;
        } else
            idx = FnLastVersion[fn]++;


        if (fn!=base_name)
            fn_borrows[fn] = fn_borrows[base_name];
        
        fn = (idx==0) ? fn : fn+"_"+std::to_string(idx); 
        CArgs.version = idx;
        CArgs.version_str = fn;
        CArgs.dyn_args = templ.dyn_args;
        FnDynArgs[fn] = templ.dyn_args;
        CArgs.cvalues = cvalues;
        
        CArgs.args = templ.args;
        CArgs.template_ret = templ.template_ret;
        fn_ret_dt[fn] = CArgs.template_ret;


        auto proto = std::make_unique<PrototypeAST>(parser_struct,
                        base_name, fn,
                        CArgs, templ);

        generics_fn.push_back({fn_ast, std::move(proto), parser_struct, fn, base_name});

        fn_ast->parser_struct->function_name = fn;
        for (auto &body : fn_ast->Body) {
            // if (begins_with(fn_name, "backprop__"))
            // std::cout << "(fn_ast codegen)" << typeid(*body).name() << "\n";
              body->Traverse([](ExprAST *node) {
                  node->Checks();
              });
        }

        // if (parser_struct->gpu>0) {
        //     int gpu = parser_struct->gpu;
        //     parser_struct->gpu = (kernel_fn.count(base_name)>0) ? 1 : 2;
        //     proto->codegen();
        //     parser_struct->gpu = gpu;
        // }

        // if (!fn_ast)
        //     LogErrorC(-1, "Template for " +base_name + " failed");
        // fn_ast->Proto = std::move(proto);
        // fn_ast->parser_struct->function_name = fn;
        // fn_ast->parser_struct->cvalues = cvalues;
        // BasicBlock *CurBB = Builder->GetInsertBlock();

        // fn_ast->codegen();

        // Builder->SetInsertPoint(CurBB);
        
        found = true;
        return fn;
    }

    return fn;
}


std::string GetFnVersion(Parser_Struct *parser_struct, std::string fn, CallArgsTy CArgs, bool &found, bool accept_layout, bool match_owned) {
    found = true;
    for (auto cargs : FnVersion[fn]) {
        // if(fn=="bar") {
        //     std::cout << "FOUND FOR " << cargs.version_str << "\n";
        //     print_dt_vec(CArgs.dts);
        //     print_dt_vec(cargs.dts);
        // }
        if(CompareDTs(cargs.dts, CArgs.dts, accept_layout, match_owned) && CArgs.cvalues==cargs.cvalues) {
            return cargs.version_str;
        }
    }
    found = false;
    return fn;
}

void FnNotFound(Parser_Struct *parser_struct, std::string fn, CallArgsTy CArgs) {
    std::cout << "\n\nFound implementations" << "\n";
    int i=0;
    for (auto cargs : FnVersion[fn]) {
        if (i++>3)
            break;
        std::cout << "\n" << cargs.version_str << "\n";
        print_dt_vec(cargs.dts);
    }
    if (i>3)
        std::cout << "...\n";
    std::cout << "\nsent:" << "\n";
    print_dt_vec(CArgs.dts);
    LogErrorS(parser_struct->line, "Unmatched call args for " + fn);
}



inline void Semantic_Arguments_Check(Parser_Struct *parser_struct,
                                  std::vector<std::unique_ptr<ExprAST>> &Args,
                                  std::string fn_name,
                                  bool is_nsk_fn, int sent_args, int arg_offset=1) {
  bool is_vararg = in_vec(fn_name, vararg_methods);

  if (Function_Required_Arg_Count.count(fn_name)>0) {
      if (sent_args<Function_Required_Arg_Count[fn_name] && !is_vararg)
          LogErrorS(parser_struct->line, "Passed " + std::to_string(sent_args) + " arguments to " + fn_name + ", but " + std::to_string(Function_Required_Arg_Count[fn_name]) + " are required.");
  }


  // -- Required Arguments -- //
  unsigned i, e;
  for (i = 0, e = Args.size(); i != e; ++i) {
    if (dynamic_cast<PositionalArgExprAST*>(Args[i].get()))
        break;
    
    Data_Tree data_type = Args[i]->GetDataTree();
    
    int tgt_arg = i + arg_offset;


    if(fn_argnames.count(fn_name)==0) {
        LogErrorS(parser_struct->line, "Function " + fn_name + " does not require arguments.");
        return;
    }
    if(tgt_arg>=fn_argnames[fn_name].size()) {
        LogErrorS(parser_struct->line, "Extrapolated " + fn_name + " arguments count. Sent at least: " + std::to_string(tgt_arg) + ", but expected " + std::to_string(fn_argnames[fn_name].size()));
        return;
    }


    if (!in_vec(fn_name, {"to_int", "to_bool",  "to_float", "printl", "print"})) { 

      if (Function_Arg_DataTypes.count(fn_name)>0) {   

        Data_Tree expected_data_type = Function_Arg_DataTypes[fn_name][fn_argnames[fn_name][tgt_arg]];



        int differences = expected_data_type.Compare(data_type);
        if (differences>0) { 
          int line = (!parser_struct) ? 0 : parser_struct->line;
          std::cout << "Expected\n   ";
          expected_data_type.Print();
          std::cout << "\nPassed\n   ";
          data_type.Print();
          std::cout << "\n\n";
          LogErrorS(line, "Got an incorrect type for argument " + fn_argnames[fn_name][tgt_arg] + " of function " + fn_name + ".");
        } 
      }
    }
  }


  i = i + arg_offset-1;
  if (gpu_fn.count(fn_name)>0||gpu_ffi.count(fn_name))
      i++;
  // -- Add Default Arguments -- //
  if (Function_Arg_Count.count(fn_name)>0&&!is_vararg) {    
      for (; i<Args.size(); ++i) { // Positional Arguments
          auto PosArg = dynamic_cast<PositionalArgExprAST*>(Args[i].get());
          if(!PosArg)
            LogErrorS(parser_struct->line, "Standard argument followed by positional argument.");
      }
  }
}




  
 
  
  /// NumberExprAST - Expression class for numeric literals like "1.0".
NumberExprAST::NumberExprAST(float Val) : Val(Val) {
  this->SetType("float");
} 
void NumberExprAST::SetIsInf(bool is_inf) {
    if (is_inf) {
        Val = std::numeric_limits<float>::max();
        IsInf = true;
    }
}


IntExprAST::IntExprAST(int64_t Val) : Val(Val) {
  this->SetType("int");
} 
void IntExprAST::SetIsInf(bool is_inf) {
    if (is_inf) {
        Val = std::numeric_limits<int32_t>::max();
        IsInf = true;
    }
}

Data_Tree ConstExprAST::GetDataTree(bool from_assignment) {
    return Data_Tree("int");
}

ConstExprAST::ConstExprAST(Parser_Struct *parser_struct, std::string str) : str(str) {
    this->parser_struct = parser_struct;
} 

LutLoExprAST::LutLoExprAST() {} 
LutHiExprAST::LutHiExprAST() {} 
Data_Tree LutLoExprAST::GetDataTree(bool from_assignment) {
    Data_Tree dt = Data_Tree("vec");
    dt.Nested_Data.push_back(Data_Tree("i8"));
    dt.Nested_Data.push_back(Data_Tree("32"));
    return dt;
} 
Data_Tree LutHiExprAST::GetDataTree(bool from_assignment) {
    Data_Tree dt = Data_Tree("vec");
    dt.Nested_Data.push_back(Data_Tree("i8"));
    dt.Nested_Data.push_back(Data_Tree("32"));
    return dt;
} 

BoolExprAST::BoolExprAST(bool Val) : Val(Val) {
  this->SetType("bool");
} 
  
  

  
StringExprAST::StringExprAST(std::string Val) : Val(Val) {
  this->SetType("str");
} 

CharExprAST::CharExprAST(int Val) : Val(Val) {
  this->SetType("char");
} 
  

NullPtrExprAST::NullPtrExprAST() {
  this->SetType("nullptr");
} 

Data_Tree VariableListExprAST::GetDataTree(bool from_assignment) {

    Data_Tree data_type = Data_Tree("tuple");
    for (auto &expr : ExprList)
        data_type.Nested_Data.push_back(expr->GetDataTree());
    return data_type;
}

VariableListExprAST::VariableListExprAST(std::vector<std::unique_ptr<Nameable>> ExprList)
            : ExprList(std::move(ExprList)) {
  this->SetIsList(true);
} 

  


  

  
  
  
  
  
/// VarExprAST - Expression class for var/in
VarExprAST::VarExprAST(
    std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
    std::string Type)
    : VarNames(std::move(VarNames)), Type(Type) {}
  
  


NewTupleExprAST::NewTupleExprAST(
    std::vector<std::unique_ptr<ExprAST>> Values)
    : Values(std::move(Values)) {}

void NewVecExprAST::Checks() {
  GetDataTree();
}

IntervalLoopExprAST::IntervalLoopExprAST(
        Parser_Struct *parser_struct,
        std::vector<std::unique_ptr<ExprAST>> Starts,
        std::vector<std::unique_ptr<ExprAST>> Ends,
        std::vector<std::unique_ptr<ExprAST>> Body,
        std::vector<std::unique_ptr<ExprAST>> VarNames,
        uint64_t scope_depth, uint64_t control_stmt_id)
        : Starts(std::move(Starts)), Ends(std::move(Ends)),
          VarNames(std::move(VarNames)) {

    // if (this->VarNames.size()==0)
    //     this->VarNames = {"i", "j", "k"};


    int loops = this->Starts.size();
    
    Parser_Struct *last_parser_struct = parser_struct;
    for (int i=loops-1; i>=0; --i) {
        std::string var_name = this->VarNames[i]->GetName();
        Parser_Struct *loop_parser_struct = last_parser_struct->Copy();

        // todo scope_depth is inversed
        loop_parser_struct->scope_depth++;
        last_parser_struct = loop_parser_struct;

        std::unique_ptr<Nameable> nameable = std::make_unique<Nameable>(last_parser_struct,
                                                                         var_name, 1);
        nameable->AddNested(std::make_unique<NameableRoot>(last_parser_struct));

        std::unique_ptr<ExprAST> EndCond = std::make_unique<BinaryExprAST>(
                '<', std::move(nameable),
                std::move(this->Ends[i]),
                last_parser_struct);


        std::vector<std::unique_ptr<ExprAST>> Loop;
        Loop.push_back(std::make_unique<ForExprAST>(
                var_name, std::move(this->Starts[i]),
                std::move(EndCond),
                std::make_unique<IntExprAST>(1),
                std::move(Body), loop_parser_struct,
                scope_depth, control_stmt_id
            ));
        Body = std::move(Loop);
    }
    this->Body = std::move(Body);

}

void IntervalLoopExprAST::Checks() {
    if (Check) return;
    Check = true;
  // typeVars[parser_struct->function_name][VarName] = Start->GetDataTree().Type;
  // data_typeVars[parser_struct->function_name][VarName] = Start->GetDataTree();
}
  
NewVecExprAST::NewVecExprAST(
    std::vector<std::unique_ptr<ExprAST>> Values,
    std::string Type)
    : Values(std::move(Values)), Type(Type) 
{
  this->SetType(Type);
}


NewDictExprAST::NewDictExprAST(
    std::vector<std::unique_ptr<ExprAST>> Keys,
    std::vector<std::unique_ptr<ExprAST>> Values,
    std::string Type, Parser_Struct *parser_struct)
    : Keys(std::move(Keys)), Values(std::move(Values)), Type(Type)
{
  this->SetType(Type);
  this->parser_struct = parser_struct;
}
  
  
void ObjectExprAST::Checks() {
    for (unsigned i = 0, e = this->VarNames.size(); i != e; ++i) {

        std::string name = this->VarNames[i].first;
        data_typeVars[parser_struct->function_name][name] = Data_Tree(ClassName);
        if (this->HasInit[i]) { // callee init
          std::string create_fn = ClassName+"___init__";
          Semantic_Arguments_Check(this->parser_struct, this->Args[i], create_fn, false, this->Args[i].size(), 1);
          FunctionChecks(create_fn);
        }  
    }
}

ObjectExprAST::ObjectExprAST(
    Parser_Struct *parser_struct,
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
  std::vector<bool> HasInit,
  std::vector<std::vector<std::unique_ptr<ExprAST>>> Args,
  std::string Type,
  std::unique_ptr<ExprAST> Init, std::string ClassName)
  :  HasInit(std::move(HasInit)), Args(std::move(Args)), VarExprAST(std::move(VarNames), std::move(Type)), Init(std::move(Init)), ClassName(ClassName)
{
    this->parser_struct = parser_struct;

    for (unsigned i = 0, e = this->VarNames.size(); i != e; ++i) {
        if (!this->HasInit[i]) {
            std::string name = this->VarNames[i].first;
            if(!this->VarNames[i].second)
                continue;
            int owned_id = this->VarNames[i].second->GetIsOwned();
            if (owned_id>=-1) 
                function_owns[parser_struct->function_name][name] = owned_id;
            int memid = this->VarNames[i].second->GetMemId();
            if (memid > -2)
                fn_memid[parser_struct->function_name][name] = memid;
        }
    }
}



void Print_Names_Str(std::vector<std::string> names_vec) {


  if(names_vec.size()==0)
    return;
  if (names_vec.size() == 1) {
    std::cout << "\n\nName: " << names_vec[0] << "\n\n\n";
    return;
  }

  
  std::cout << "\n\nName 2 strs: ";
  for (int i=0; i<=names_vec.size()-2; ++i)
    std::cout << names_vec[i] << "."; 
  std::cout << names_vec[names_vec.size()-1] << "\n\n\n"; 
}


NameableExprAST::NameableExprAST() {}

SelfExprAST::SelfExprAST() {
  Expr_String = {"self"};
  End_of_Recursion=true;
  Name="self";
  height=1;
  From_Self=true;
}
EmptyStrExprAST::EmptyStrExprAST() {
  Expr_String = {};
  End_of_Recursion=true;
  height=0;
}

NestedVectorIdxExprAST::NestedVectorIdxExprAST(std::unique_ptr<NameableExprAST> Inner_Expr, std::string name, Parser_Struct *parser_struct, std::unique_ptr<IndexExprAST> Idx, std::string type)
                                        :  Idx(std::move(Idx)) {
  this->parser_struct = parser_struct;
  this->Inner_Expr = std::move(Inner_Expr);
  this->Inner_Expr->IsLeaf=false;
  this->Name = name;
  this->SetType(type);
  
  height=this->Inner_Expr->height+1;

  Expr_String = this->Inner_Expr->Expr_String;
  Expr_String.push_back(name);
  // Print_Names_Str(Expr_String);
}



NestedCallExprAST::NestedCallExprAST(std::unique_ptr<NameableExprAST> Inner_Expr, std::string Callee, Parser_Struct *parser_struct,
  std::vector<std::unique_ptr<ExprAST>> Args)
  : Inner_Expr(std::move(Inner_Expr)), Callee(Callee), Args(std::move(Args)) {
}


NestedVariableExprAST::NestedVariableExprAST(std::unique_ptr<NameableExprAST> Inner_Expr, Parser_Struct *parser_struct, std::string type, Data_Tree data_type)
      : Inner_Expr(std::move(Inner_Expr)) {
  this->SetType(type);

  this->Name = this->Inner_Expr->Name;
  this->data_type = data_type;
}
 
void UnkVarExprAST::Checks() {
  for (unsigned i = 0, e = this->VarNames.size(); i != e; ++i) {
    const std::string &VarName = this->VarNames[i].first; 
    ExprAST *Init = this->VarNames[i].second.get();

    Data_Tree dt = Init->GetDataTree();

    if(Init->GetIsMsg()) {
      if(dt.Nested_Data.size()==0)
        LogBlue("Failed to receive message from " + Init->GetName() + ". Is it a channel?");
      dt = dt.Nested_Data[0];
    }
        
    // if (Object_toClass[parser_struct->function_name].count(VarName)>0\
    //   ||data_typeVars[parser_struct->function_name].count(VarName)>0) {
    //     LogErrorS(parser_struct->line, "Redefinition of " + VarName);
    //     continue;
    // }


    data_typeVars[parser_struct->function_name][VarName] = dt;
  }



}

UnkVarExprAST::UnkVarExprAST(
  Parser_Struct *parser_struct,
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
  std::string Type,
  std::vector<std::unique_ptr<ExprAST>> Notes)
  : VarExprAST(std::move(VarNames), std::move(Type)),
                Notes(std::move(Notes)) {
  this->parser_struct = parser_struct;

  for(auto &[name, expr] : this->VarNames) {
    int owned_id = expr->GetIsOwned();
    if (owned_id>=-1)
        function_owns[parser_struct->function_name][name] = owned_id;
    int memid = expr->GetMemId();
    if (memid > -2)
        fn_memid[parser_struct->function_name][name] = memid; 
  }
}

bool UnkVarExprAST::GetNeedGCSafePoint() {
    return true;
}


TupleExprAST::TupleExprAST(
  Parser_Struct *parser_struct,
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
  std::string Type,
  Data_Tree data_type) : VarExprAST(std::move(VarNames), std::move(Type)), data_type(data_type) {

  this->parser_struct = parser_struct;

    
  for (unsigned i = 0, e = this->VarNames.size(); i != e; ++i) {
    const std::string &VarName = this->VarNames[i].first; 
    ExprAST *Init = this->VarNames[i].second.get();

    Data_Tree other_type = Init->GetDataTree();
    
    if(this->Type=="tuple")
      Check_Is_Compatible_Data_Type(data_type, other_type, parser_struct);
    
    data_typeVars[parser_struct->function_name][VarName] = data_type;
  }
}

ListExprAST::ListExprAST(
  Parser_Struct *parser_struct,
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
  std::string Type,
  Data_Tree data_type) : VarExprAST(std::move(VarNames), std::move(Type)), data_type(data_type) {

  this->parser_struct = parser_struct;

  for (unsigned i = 0, e = this->VarNames.size(); i != e; ++i) {
    const std::string &VarName = this->VarNames[i].first; 
    ExprAST *Init = this->VarNames[i].second.get();

    Data_Tree other_type = Init->GetDataTree();
    
    Check_Is_Compatible_Data_Type(data_type, other_type, parser_struct);
    
    data_typeVars[parser_struct->function_name][VarName] = data_type;
  }
}

DictExprAST::DictExprAST(
  Parser_Struct *parser_struct,
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
  std::string Type,
  Data_Tree data_type) : VarExprAST(std::move(VarNames), std::move(Type)), data_type(data_type) {

    
  this->parser_struct = parser_struct;

  for (unsigned i = 0, e = this->VarNames.size(); i != e; ++i) {
    const std::string &VarName = this->VarNames[i].first; 
    ExprAST *Init = this->VarNames[i].second.get();

    Data_Tree other_type = Init->GetDataTree();
    
    Check_Is_Compatible_Data_Type(data_type, other_type, parser_struct);
    
    data_typeVars[parser_struct->function_name][VarName] = data_type;
  }
}


void DataExprAST::Checks() {
  for (unsigned i = 0, e = this->VarNames.size(); i != e; ++i) {
    if(this->isSelf)
      continue;    

    const std::string &VarName = this->VarNames[i].first;  
    ExprAST *Init = this->VarNames[i].second.get();

    Data_Tree init_dt = Init->GetDataTree();
    std::string init_type = init_dt.Type;

    Check_Is_Compatible_Data_Type(data_type, init_dt, parser_struct);


    data_typeVars[parser_struct->function_name][VarName] = data_type;
    typeVars[parser_struct->function_name][IdentifierStr] = data_type.Type;


    create_fn = this->Type;
    create_fn = (create_fn=="tuple") ? "list" : create_fn;
    create_fn = create_fn + "_Create";

    DtHasCreateFn = (!data_type.is_array && (struct_create_fn.count(dt_type)>0 || TheModule->getFunction(create_fn)!=nullptr));

    if(DtHasCreateFn) {
      if (auto *null_stmt = dynamic_cast<NullPtrExprAST*>(this->VarNames[i].second.get())) {
            if (!(create_fn=="array_Create" || create_fn=="map_Create"))
              Semantic_Arguments_Check(parser_struct, this->Notes, create_fn, true, this->Notes.size(), 1);
      }
    }
  }

}
  
DataExprAST::DataExprAST(
  Parser_Struct *parser_struct,
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
  std::string Type, Data_Tree data_type, bool HasNotes, bool IsStruct,
  bool IsOwned,
  std::vector<std::unique_ptr<ExprAST>> Notes)
  : VarExprAST(std::move(VarNames), std::move(Type)), data_type(data_type), HasNotes(HasNotes), IsOwned(IsOwned), IsStruct(IsStruct),
                Notes(std::move(Notes)) {   
  this->parser_struct = parser_struct;
  dt_type = "DT_"+data_type.Type;  

  if(data_type.Type=="charv") {      
      int size;
      if(this->Notes.size()<1)
            LogErrorS(parser_struct->line, "charv requires size argument");
      if (auto num_expr = dynamic_cast<IntExprAST*>(this->Notes[0].get()))
            size = num_expr->Val;
      else
            LogErrorS(parser_struct->line, "charv size must be int");

      data_type.Nested_Data.push_back(Data_Tree(std::to_string(size)));
  }

  SetMemId();
}

void DataExprAST::SetMemId() {
  if (!data_type.IsFromArena())
      return;

  for(auto &[name, expr] : this->VarNames) {
    if (IsOwned&&dynamic_cast<NullPtrExprAST*>(expr.get())) {
        function_owns[parser_struct->function_name][name]=(*parser_struct->owned_id)++;
        fn_memid[parser_struct->function_name][name] = (*parser_struct->mem_id)++;
    } else {
        int owned_id = expr->GetIsOwned();
        if (owned_id>=-1)
            function_owns[parser_struct->function_name][name] = owned_id;
        int memid = expr->GetMemId();
        if (memid > -2)
            fn_memid[parser_struct->function_name][name] = memid;
    }
  }
}






bool DataExprAST::GetNeedGCSafePoint() {
    return true;
}

Data_Tree NewExprAST::GetDataTree(bool from_assignment) {
    if(data_type.Type!="")
        return data_type;

    // High-level class
    if (fn_ret_dt.count(Callee)==0) {
        Callee = DataName + "___init__";
        if (Classes.count(DataName)==0)
            LogErrorS(parser_struct->line, "New not implemented for data type " + DataName);
        is_high_level_obj = true;
        data_type = Data_Tree(DataName);
        data_type.is_own = MemoryType;
        FunctionChecks(Callee);
        return data_type;
    }

    // Other data types (DT_<data>)
    Data_Tree new_dt = fn_ret_dt[Callee];
    data_type = new_dt;
    data_type.is_own = MemoryType;
    return new_dt;
}

void NewExprAST::Checks() {
    if (checked)
        return;
    checked=true;
    GetDataTree();
}

NewExprAST::NewExprAST(Parser_Struct *parser_struct, std::string DataName, std::vector<std::unique_ptr<ExprAST>> Args, int memory_type)
            : DataName(DataName), Args(std::move(Args)), MemoryType(memory_type) {
    this->parser_struct = parser_struct;
    Callee = DataName + "_Create";
    // GetDataTree();


    if (this->MemoryType!=0)
        OwnedId = (*parser_struct->owned_id)++;
    MemId = (*parser_struct->mem_id)++;
}

bool NewExprAST::GetNeedGCSafePoint() {
    return true;
}

LibImportExprAST::LibImportExprAST(std::string LibName, bool IsDefault, Parser_Struct *parser_struct)
  : LibName(LibName), IsDefault(IsDefault) {

  this->parser_struct = parser_struct;


  std::string ai_path = LibName+".nv";

  if (!(in_vec(LibName, imported_libs))) {
    
    bool has_nv=false, has_so_lib=false;
    std::string lib_path = std::getenv("NEVE_LIBS");

    std::string lib_dir = lib_path + "/" + LibName;
    std::string so_lib_path = lib_dir + "/lib.so";
    if (in_vec(so_lib_path, imported_libs))
        return;
    if(fs::exists(so_lib_path)) {
      has_so_lib=true;
      LibParser *lib_parser = new LibParser(lib_dir);
      
      lib_parser->ParseLibs();
      lib_parser->ImportLibs(so_lib_path, LibName, IsDefault);

      imported_libs.push_back(LibName);
    }




    std::string include_path = lib_dir + "/include.nv";
    if (in_vec(include_path, imported_libs))
        return;
    if(fs::exists(include_path)) {
      has_nv=true;
      get_tok_until_space();
      import_NEVE_File(include_path);
      imported_libs.push_back(include_path);
    } else 
      getNextToken(); // eat lib name
    



    if(!(has_nv||has_so_lib))
      LogErrorS(parser_struct->line, "Failed to import library: " + LibName + ".\n\t    Could not find .nv or lib.so file.");
    else
      imported_libs.push_back(LibName);
  }
}

  
  
  
Data_Tree ReduceExprAST::GetDataTree(bool from_assignment) {
    bool is_reduce = functional_type == "reduce";
    auto dt = LHS->GetDataTree();

    std::string type = dt.Type;
    if (dt.Type=="layout") {
        std::string inner = dt.Nested_Data[0].Type;
        fn = inner + "_buffer";
        if(is_reduce)
            return Data_Tree(inner);
        else
            return dt;

    }
    if (dt.is_array||dt.is_buffer) {
        fn = type + "_buffer";
        if(is_reduce)
            return Data_Tree(type);
        else
            return dt;
    }
    if (type=="array") {
        std::string inner = dt.Nested_Data[0].Type;
        fn = "array_"+inner;
        if(is_reduce)
            return dt.Nested_Data[0];
        else
            return dt;
    }
    if (type=="map") {
        std::string inner = dt.Nested_Data[1].Type;
        fn = "map_"+inner;
        if(is_reduce)
            return dt.Nested_Data[1];
        else
            return dt;
    }
    return dt;
}
  

void ReduceExprAST::Checks() {
    GetDataTree();

    std::string gpu_str = (parser_struct->gpu>0)  ? "gpu_" : "";
    std::string callee = fn + "_" + gpu_str + functional_type + "_" + op_map[Op];
    FunctionChecks(callee);
}
  
ReduceExprAST::ReduceExprAST(Parser_Struct *parser_struct, std::unique_ptr<ExprAST> LHS,
                             char Op, std::string functional_type)
            : LHS(std::move(LHS)),
              Op(Op), functional_type(functional_type) {
    this->parser_struct = parser_struct;
}
  
  
  
LambdaExprAST::LambdaExprAST(Parser_Struct *parser_struct, std::string lambda_fn, std::vector<std::string> Args)
    : lambda_fn(lambda_fn), Args(std::move(Args)) {
    this->parser_struct = parser_struct;
}

  
  
Data_Tree MapitExprAST::GetDataTree(bool from_assignment) {
    Checks();
    return LHS->GetDataTree();
}


void MapitExprAST::Checks() {
    if (Check)
        return;
    Check=true;


    Data_Tree dt = this->LHS->GetDataTree();
    std::string scope = this->Lambda->lambda_fn;

    

    int first_idx;
    if (parser_struct->gpu>0)
        first_idx = 0;
    else {
        first_idx = 1;
        this->Lambda->ArgsType.push_back(Data_Tree("any"));
    }

    if (dt.is_buffer||dt.is_array) {
        fn = dt.Type + "_buffer";
        this->Lambda->ArgsType.push_back(Data_Tree(dt.Type));
        data_typeVars[scope][this->Lambda->Args[first_idx]] = Data_Tree(dt.Type);
    }
    if (dt.Type=="array") {
        fn = "array_" + dt.Nested_Data[0].Type;
        this->Lambda->ArgsType.push_back(dt.Nested_Data[0]);
        data_typeVars[scope][this->Lambda->Args[first_idx]] = dt.Nested_Data[0];
    }
    if (dt.Type=="map") {
        fn = "map_" + dt.Nested_Data[1].Type;
        this->Lambda->ArgsType.push_back(dt.Nested_Data[1]);
        data_typeVars[scope][this->Lambda->Args[first_idx]] = dt.Nested_Data[1];
    }
    
    std::string gpu_str = (parser_struct->gpu>0) ? "_gpu" : "";
    fn+=gpu_str+"_mapit";
}

MapitExprAST::MapitExprAST(Parser_Struct *parser_struct, std::unique_ptr<ExprAST> LHS, std::unique_ptr<LambdaExprAST> Lambda)
    : LHS(std::move(LHS)), Lambda(std::move(Lambda)) {
    this->parser_struct = parser_struct;

}
  

Data_Tree LayoutExprAST::GetDataTree(bool) {
    dt = Data_Tree("layout");
    dt.Nested_Data.push_back(Data_Tree(data_type_to_name()[type]));
    for (int i=0; i<CArgs.size(); ++i) {
        auto &expr = CArgs[i]->expr;
        std::string name = (expr) ? CArgs[i]->expr->Name : CArgs[i]->name;
        dt.Nested_Data.push_back(Data_Tree(name));
    }
    return dt;
}




LayoutExprAST::LayoutExprAST(Parser_Struct *parser_struct, uint16_t type,
        std::vector<std::unique_ptr<CompiledArgs>> CArgs, std::vector<std::unique_ptr<ExprAST>> Args, bool smem) 
    : type(type), CArgs(std::move(CArgs)),
      Args(std::move(Args)), smem(smem) {
    this->parser_struct = parser_struct;
    GetDataTree();
}


FnCompiledValues HandleCompiledArgs(Parser_Struct *parser_struct,
        std::string fn_name, CallArgsTy CompiledArgsVec) {

    FnCompiledValues fn_compiled_values; 
    if (Fn_Compiled_Args.count(fn_name)==0) {
        fn_compiled_values.has=false;
        return fn_compiled_values;
    }

    auto compiled_args = &Fn_Compiled_Args[fn_name];
    
    int last_int=0, last_float=0, last_string=0;
    
    for (auto &arg : *compiled_args) {
        std::string type = arg->dt.Type;
        std::string name = arg->name;
        if (type=="int")
            fn_compiled_values.AddInt(name, CompiledArgsVec.ints[last_int++]);
        else
            std::cout << "unimplemented comptime pass " << type << "\n";
    }

    return fn_compiled_values;
}

void LaunchExprAST::Checks() {
    Semantic_Arguments_Check(parser_struct, Args, fn_name, false, Args.size(), 0);
}

LaunchExprAST::LaunchExprAST(Parser_Struct*, std::unique_ptr<ExprAST> Grid,
        std::unique_ptr<ExprAST> Block,
        std::unique_ptr<ExprAST> Smem,
        std::unique_ptr<ExprAST> Stream,
        std::vector<std::unique_ptr<ExprAST>> Args,
        CallArgsTy CompiledArgsVec,
        std::string fn_name) 
    : Grid(std::move(Grid)), Block(std::move(Block)), Args(std::move(Args)),
      Smem(std::move(Smem)), Stream(std::move(Stream)),
      CompiledArgsVec(CompiledArgsVec),
      fn_name(fn_name) {


    if (auto stmt = dynamic_cast<NewVecExprAST*>(this->Grid.get())) {
        for (int i=stmt->Values.size(); i<3; i++)
            stmt->Values.insert(stmt->Values.end(), std::make_unique<IntExprAST>(1));
    }

    if (auto stmt = dynamic_cast<NewVecExprAST*>(this->Block.get())) {
        for (int i=stmt->Values.size(); i<3; i++)
            stmt->Values.insert(stmt->Values.end(), std::make_unique<IntExprAST>(1));
    }

    
    CompiledArgs = HandleCompiledArgs(parser_struct, fn_name, CompiledArgsVec);

    
    if (CompiledArgs.has)
        fn_name = mangle_cargs_proto(fn_name);
}
  
  
Data_Tree UnaryExprAST::GetDataTree(bool from_assignment) {
    if (Opcode=='!')
        return Data_Tree("bool");
    return Operand->GetDataTree();
}



  /// UnaryExprAST - Expression class for a unary operator.
UnaryExprAST::UnaryExprAST(int Opcode, std::unique_ptr<ExprAST> Operand, Parser_Struct *parser_struct)
    : Opcode(Opcode), Operand(std::move(Operand)) {
  this->parser_struct = parser_struct;
}
  
bool UnaryExprAST::GetNeedGCSafePoint() {
    return Operand->GetNeedGCSafePoint();
}
  

inline void correspondTemplateType(Data_Tree &arg_dt, Data_Tree &sent_dt,
        std::string &key,
        std::map<std::string, std::string> &ret_map, bool &found) {
    if (arg_dt.Type==key) {
        found=true;
        ret_map[key] = sent_dt.Type;
        return;
    }
    for (int i=0; i<arg_dt.Nested_Data.size();++i) {
        correspondTemplateType(arg_dt.Nested_Data[i], sent_dt.Nested_Data[i],
                key, ret_map, found);
        if (found)
            return;
    }
}

inline void buildTemplateOpCorrespondence(Parser_Struct *parser_struct,
        std::string op, Data_Tree &dt, Data_Tree &L_dt, Data_Tree &R_dt,
        std::map<std::string, std::string> &ret_map) {

    std::string type = dt.Type;
    if (!in_vec(type,data_tokens)&&!in_vec(type,compound_tokens)&&type!="layout") {
        auto proto = FunctionProtos[op].get();
        bool found=false;
        correspondTemplateType(proto->Types[0], L_dt, type, ret_map, found);
        if (!found)
            correspondTemplateType(proto->Types[1], R_dt, type, ret_map, found);
    }

    for (int i=0; i<dt.Nested_Data.size(); ++i) {
        buildTemplateOpCorrespondence(parser_struct, op, dt.Nested_Data[i], L_dt, R_dt, ret_map);
    }
}

void GetSubmitedCValues_Recursive(FnCompiledValues &cvalues,
                    Parser_Struct *parser_struct, Data_Tree dt) {
    if (parser_struct->cvalues.dts.count(dt.Type)>0) {
        std::string type = parser_struct->cvalues.dts[dt.Type].Type;
        if (type=="int")
            cvalues.AddInt(dt.Type, parser_struct->cvalues.ints[dt.Type]);
    }
    for (auto &inner_dt : dt.Nested_Data)
        GetSubmitedCValues_Recursive(cvalues, parser_struct, inner_dt);
}

FnCompiledValues BinaryExprAST::GetSubmitedCValues() {
    FnCompiledValues cvalues;
    GetSubmitedCValues_Recursive(cvalues, parser_struct, L_dt);
    GetSubmitedCValues_Recursive(cvalues, parser_struct, R_dt);
    return cvalues;
}



Data_Tree BinaryExprAST::GetDataTree(bool from_assignment) {
  std::string operation = op_map[Op];
  L_dt = LHS->GetDataTree(Op=='=');
  R_dt = RHS->GetDataTree();

  if (R_dt.Type=="function") {
      FunctionChecks(RHS->GetName());
  }

  std::string LType = UnmangleVec(L_dt), RType = UnmangleVec(R_dt);
  if(ends_with(LType, "channel"))
    LType = "channel";
  if(ends_with(RType, "channel"))
    RType = "channel";

  if ((LType=="list"||RType=="list") && Op!='=')
    LogErrorS(parser_struct->line, "Tuple elements type are unknown during parsing type. Please load the element into a static type variable first.");


  
  if (LType=="layout") {
      if (auto *stmt = dynamic_cast<NameableIdx*>(LHS.get())) {
        L_dt = stmt->GetLayoutDT(L_dt, Op=='=');
        LType = UnmangleVec(L_dt);
      }
  }
  if (RType=="layout") {
      if (auto *stmt = dynamic_cast<NameableIdx*>(RHS.get())) {
        R_dt = stmt->GetLayoutDT(R_dt, Op=='=');
        RType = UnmangleVec(R_dt);
      }
  }



  if (LType=="char")
      LType = "i8";
  if (RType=="char")
      RType = "i8";


  Elements = LType + "_" + RType;    
  if (Elements=="float_int"&&Op!=tok_offby)
    cast_R_to="int_to_float";


  if(auto *LHSV = dynamic_cast<NameableIdx *>(this->LHS.get())) {}
  else {
      if (L_dt.is_buffer)
        LType = "buffer_"+LType;
  }
  if(auto *RHSV = dynamic_cast<NameableIdx *>(this->RHS.get())) {}
  else {
      if (R_dt.is_buffer)
        RType = "buffer_"+RType;
  }




  if (L_dt.is_array)
    LType = "buffer_"+LType;
  if (R_dt.is_array)
    RType = "buffer_"+RType;

  bool has_generic = (L_dt.Type=="layout"||R_dt.Type=="layout");
  Elements = LType + "_" + RType;    


  // casts
  if (LType!=RType&&in_vec(LType, int_types)&&in_vec(RType, int_types)) {
    //Cast to L int type
    Elements = LType+"_"+LType;
    cast_R_to = "to_"+LType;
  }
  if (Elements=="int_float") {
    Elements = "float_float"; 
    cast_L_to="int_to_float";
  }
  if (Elements=="float_int") {
    Elements = "float_float"; 
  }
  if (Elements=="float_bool") {
    Elements = "float_float"; 
    cast_R_to="bool_to_float";
  }
  if (Elements=="int_bool") {
    Elements = "int_int"; 
    cast_R_to="bool_to_int";
  }
  if (in_vec(LType, {"str", "charv"})) {
      if (RType!="int"&&in_vec(RType, int_types)) {
          cast_R_to="to_int";
          Elements = LType+"_int";
      }
  }
  Operation = Elements + "_" + operation;
  // std::cout << Elements << " | " << Operation << "\n";


  if (LType=="channel"
          &&!in_str(RType, primary_data_tokens)&&RType!="str")
    Operation = "channel_void_message";

  // std::cout << "\n";
  //   std::cout << "op" << "\n";
  //   L_dt.Print();
  //   std::cout << Operation << "\n";

  FunctionChecks(Operation);

  if (RType=="channel" && !in_str(LType, primary_data_tokens)&&LType!="str")
    Operation = "void_channel_message";
  std::string type;
  if (Operation=="int_int_div")
    type = "float";
  else if (LType=="layout"&&Op==tok_offby) {
    Data_Tree ret_dt = Data_Tree(L_dt.Nested_Data[0]);
    ret_dt.is_buffer=true;
    return ret_dt;
  }
  else if (Elements=="vec_vec")
      return L_dt;
  else if (Elements=="vec_int")
      return L_dt;
  else if (Elements=="int_vec")
      return R_dt;
  else if (ops_type_return.count(Operation)>0)
    return Data_Tree(ops_type_return[Operation]);
  else if (fn_ret_dt.count(Operation)&&!has_generic) {
    Data_Tree dt = fn_ret_dt[Operation];
    return dt;
  }
  else if (elements_type_return.count(Elements)>0)
    type = elements_type_return[Elements];
  else if (is_store_sugar) {
      if (auto *rstmt = dynamic_cast<BinaryExprAST*>(RHS.get()))
          rstmt->is_fused = (rstmt->Op=='@');
      return L_dt;
  }
  else {
      if (Op!='=') {
          std::string fn = (has_generic) ? Operation : operation; 

          if (Template_FnAST.count(fn)>0) {
            bool found;
            is_fused = (Parent!=nullptr&&Op=='@');

            std::vector<Data_Tree> Types = {L_dt, R_dt};
            if (is_fused) {
                if (auto *parent_stmt = dynamic_cast<BinaryExprAST*>(Parent)) {
                    Data_Tree parent_dt = parent_stmt->LHS->GetDataTree();
                    Types.push_back(parent_dt);
                }
            }
            // std::cout << "FUSED " << "\n";
            // L_dt.Print();
            // R_dt.Print();

            CallArgsTy CArgs = CallArgsTy(Types);
            CArgs.cvalues = GetSubmitedCValues();
            Operation = GetFnVersion(parser_struct, Operation, CArgs, found, true, true);

            if (!found) {
                Operation = GenTemplate(parser_struct, fn, CArgs, found, !has_generic);
            }
            DynamicArgs = GetDynamicArgs(parser_struct, fn, CArgs, found);
            FunctionChecks(Operation);

            if (found)
                return fn_ret_dt[Operation];
          }
          LogErrorS(parser_struct->line, "Operation function " + Operation + " not found.");
      }
  }

  return Data_Tree(type);
}

bool IsPositionalArg(Parser_Struct *parser_struct, std::string name) {
    if (ArgsInit.count(parser_struct->parse_fn)>0) {
        if (ArgsInit[parser_struct->parse_fn].count(name)> 0)
            return true;
    }
    return false;
}

  


void BinaryExprAST::Checks() {
  GetDataTree();

  std::string LType = L_dt.Type;
  std::string Lname = this->LHS->GetName();

  if (IsPositionalArg(parser_struct, Lname))
    return;
  

  std::string RType = R_dt.Type;
  // std::cout << LType << "|" << RType << " -- " << Op << "\n";


  // --- Handle store --- //
  if (Op == '=' || (Op==tok_arrow&&!begins_with(Elements, "channel"))) {


    // ch <-
    if(Op==tok_arrow) {
      if(ChannelDirections[parser_struct->function_name].count(this->RHS->GetName())==0)
        LogErrorS(parser_struct->line, "Could not find channel " + this->RHS->GetName());
      if(ChannelDirections[parser_struct->function_name][this->RHS->GetName()]==ch_receiver)
        LogErrorS(parser_struct->line, "Trying to unpack data from a receiver only channel.");
      
    }
    if (this->LHS->GetIsList()) {
        Check_Is_Compatible_Data_Type(L_dt, R_dt, parser_struct);
        return;
    }

    else if(auto *LHSV = dynamic_cast<NameableIdx *>(this->LHS.get())) {

        Data_Tree dt = LHSV->GetDataTree(true);
        LType = dt.Type;

        // map["x"] = y
        if(LType=="map") {
            Data_Tree map_dt = dt;
            Check_Is_Compatible_Data_Type(L_dt, R_dt, parser_struct);
            std::string key_type = map_dt.Nested_Data[0].Type;
            std::string query_type = LHSV->Idx->GetDataTree().Type;


            if (!(query_type=="int"&&key_type=="float")&&\
                  query_type!=key_type)
                    LogErrorS(parser_struct->line, "Querying " + key_type + " map with " + LHSV->Idx->GetDataTree().Type);
        }

        // arr[x] = y
        if (LType=="array")
          Check_Is_Compatible_Data_Type(L_dt, R_dt, parser_struct);
    }

    // is_alloca
    else if(!this->LHS->GetSelf()&&!this->LHS->GetIsAttribute()) {
      Check_Is_Compatible_Data_Type(L_dt, R_dt, parser_struct);

      if (data_typeVars[parser_struct->function_name].count(Lname)==0)
          LogErrorS(parser_struct->line, "Variable " + Lname + " not yet declared");
      
    } else
        Check_Is_Compatible_Data_Type(L_dt, R_dt, parser_struct);

    return;
  }


  // --- Handle operation --- //
  
  CheckIsSenderChannel(Elements, parser_struct, Lname);

  if(L_dt.Type=="channel"||R_dt.Type=="channel")
    Check_Is_Compatible_Data_Type(L_dt, R_dt, parser_struct);
}


BinaryExprAST::BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
              std::unique_ptr<ExprAST> RHS, Parser_Struct *parser_struct)
    : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {
  this->parser_struct = parser_struct;

  int memid = this->RHS->GetMemId();
  if (Op=='='&& memid>=-1) {
    if (auto *nameable = dynamic_cast<Nameable*>(this->LHS.get())) {
        if (nameable->Depth==1) {
            std::string name = nameable->GetName(); 
          int owned_id = this->RHS->GetIsOwned();
          if (owned_id > -2)
            function_owns[parser_struct->function_name][name] = owned_id;
          fn_memid[parser_struct->function_name][name] = memid;
        }
    }
  }
}
  
  
  





  
  
  
  
  
 
  




void RetExprAST::Checks() {
    return_expected_type = fn_ret_dt[parser_struct->function_name];

    if (this->Vars.size()==1) {
        returning_type = this->Vars[0]->GetDataTree();

        if (!Check_Is_Compatible_Data_Type(return_expected_type, returning_type, parser_struct))
            std::cout << "*Incompatible return type" << ".\n\n\n";
    }
}


RetExprAST::RetExprAST(std::vector<std::unique_ptr<ExprAST>> Vars, Parser_Struct *parser_struct, bool clear_owned)
    : Vars(std::move(Vars)), ClearOwned(clear_owned) {
    this->parser_struct = parser_struct;
}
    
  
fn_descriptor::fn_descriptor(const std::string &Name, const std::string &Return) : Name(Name), Return(Return) {}

ClassExprAST::ClassExprAST(Parser_Struct *parser_struct, const std::string &Name, const std::vector<fn_descriptor> &Functions)
  : Name(Name), Functions(Functions) {
    this->parser_struct = parser_struct;
}


// nlohmann::json ClassExprAST::toJSON() {
//   std::cout << "class to json" << ".\n";
//   nlohmann::json j;
//   j["type"] = "class";
//   j["name"] = Name;

//   j["fields"] = nlohmann::json::array();
//   for (const auto &pair : typeVars[Name]) {
//     nlohmann::json type_j;
//     type_j["name"] = pair.first;
//     type_j["type"] = pair.second;
//     j["fields"].push_back(type_j);
//   }


//   j["fields"] = nlohmann::json::array();
//   for (const auto &pair : typeVars[Name]) {
//     nlohmann::json type_j;
//     type_j["name"] = pair.first;
//     type_j["type"] = pair.second;
//     j["fields"].push_back(type_j);
//   }

//   j["methods"] = nlohmann::json::array();
//   for (const auto &fn : Functions) {
//     nlohmann::json method_j;
//     method_j["args"] = nlohmann::json::array();
//     for (int i=0; i<fn.ArgNames.size(); ++i) {
//       nlohmann::json arg_j;
//       arg_j["name"] = fn.ArgNames[i];
//       arg_j["type"] = fn.ArgTypes[i];
//       method_j["args"].push_back(arg_j);
//     }
//     method_j["name"] = fn.Name;
//     method_j["return"] = fn.Return;
//     j["methods"].push_back(method_j);
//   }

//   return j;
// }
  
GCSafePointExprAST::GCSafePointExprAST(Parser_Struct *parser_struct) {
  this->parser_struct = parser_struct;
}
  
void ForExprAST::Checks() {
  typeVars[parser_struct->function_name][VarName] = Start->GetDataTree().Type;
  data_typeVars[parser_struct->function_name][VarName] = Start->GetDataTree();
  for (auto &body : Body)
      body->Checks();
}

void ForEachExprAST::Checks() {
  data_type = Vec->GetDataTree();
  if(data_type.Nested_Data.size()==0) {
    LogError(parser_struct->line, "Using a non-compound data type at a \"for in\" expression.");
    data_type.Print();
  }
  data_typeVars[parser_struct->function_name][VarName] = data_type.Nested_Data[0];
  Type = data_type.Nested_Data[0].Type;
  for (auto &body : Body)
      body->Checks();
}

void IfExprAST::Checks() {
    for (auto &body : Then)
        body->Checks();
    for (auto &body : Else)
        body->Checks();
}  

/// IfExprAST - Expression class for if/then/else.
IfExprAST::IfExprAST(Parser_Struct *parser_struct,
          std::unique_ptr<ExprAST> Cond,
          std::vector<std::unique_ptr<ExprAST>> Then,
          std::vector<std::unique_ptr<ExprAST>> Else,
          uint64_t scope_depth, uint64_t control_stmt_id, uint64_t branch_id)
    : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {
  this->parser_struct = parser_struct;

  uint64_t depth = scope_depth+1;
  branch_id = (depth<<48) | (control_stmt_id << 32) | (2 << 16) | branch_id;
  BranchId = branch_id;



  for (auto &body : this->Then) {
      if (body->BranchId>2)
          continue;
      // no skip, may overwrite the if branch if not found
      body->Traverse([&branch_id](ExprAST *node) {
        ExprSetBranch(node, branch_id);
      });
  }
  for (auto &body : this->Else) {
      body->Traverse([&branch_id](ExprAST *node) {
        ExprSetBranch(node, branch_id);
      });
  }


  ExprTieBranch(parser_struct, this->Then, control_stmt_id);
  ExprTieBranch(parser_struct, this->Else, control_stmt_id);
}
  
  
/// ForExprAST - Expression class for for.
ForExprAST::ForExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Start,
          std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step,
          std::vector<std::unique_ptr<ExprAST>> Body,
          Parser_Struct *parser_struct,
          uint64_t scope_depth, uint64_t control_stmt_id)
    : VarName(VarName), Start(std::move(Start)), End(std::move(End)),
      Step(std::move(Step)), Body(std::move(Body)) {
    this->parser_struct = parser_struct;

  uint64_t depth = scope_depth+1;
  uint64_t branch_id = (depth<<48) | (control_stmt_id << 32) | (1<<16) | (uint64_t)2;
  BranchId = branch_id;


  for (auto &body : this->Body) {
      if (body->BranchId>2)
          continue;
      body->Traverse([&branch_id](ExprAST *node) {
        ExprSetBranch(node, branch_id);
      });
  }

  ExprTieBranch(parser_struct, this->Body, control_stmt_id);
}

  

/// ForExprAST - Expression class for for.
ForEachExprAST::ForEachExprAST(const std::string &VarName,
          std::unique_ptr<ExprAST> Vec,
          std::vector<std::unique_ptr<ExprAST>> Body, 
          Parser_Struct *parser_struct,
          uint64_t scope_depth, uint64_t control_stmt_id)
    : VarName(VarName), Vec(std::move(Vec)), Body(std::move(Body)) {
    this->parser_struct = parser_struct;
    this->data_type = data_type;
    typeVars[parser_struct->function_name][VarName] = "foreach_control_var";

  uint64_t depth = scope_depth+1;
  uint64_t branch_id = (depth<<48) | (control_stmt_id << 32) | (1<<16) | (uint64_t)2;
  BranchId = branch_id;

  for (auto &body : this->Body) {
      if (body->BranchId>2)
          continue;
      body->Traverse([&branch_id](ExprAST *node) {
        ExprSetBranch(node, branch_id);
      });
  }

    ExprTieBranch(parser_struct, this->Body, control_stmt_id);
}

void MainExprAST::Checks() {
  for (auto &body : Bodies)
      body->Checks();
}
void WhileExprAST::Checks() {
  for (auto &body : Body)
      body->Checks();
}

  /// WhileExprAST - Expression class for while.
WhileExprAST::WhileExprAST(std::unique_ptr<ExprAST> Cond, std::vector<std::unique_ptr<ExprAST>> Body, Parser_Struct *parser_struct,
        uint64_t scope_depth, uint64_t control_stmt_id)
  : Cond(std::move(Cond)), Body(std::move(Body)) {
    this->parser_struct = parser_struct;

  uint64_t depth = scope_depth+1;
  uint64_t branch_id = (depth<<48) | (control_stmt_id << 32) | (1<<16) | (uint64_t)2;
  BranchId = branch_id;
  for (auto &body : this->Body) {
      if (body->BranchId>2)
          continue;
      body->Traverse([&branch_id](ExprAST *node) {
        ExprSetBranch(node, branch_id);
      });
  }
  ExprTieBranch(parser_struct, this->Body, control_stmt_id);
}

BreakExprAST::BreakExprAST() {}
ContinueExprAST::ContinueExprAST() {}

IndexExprAST::IndexExprAST(std::vector<DimSlice> Idxs)
            : Idxs(std::move(Idxs)) {
  Size = this->Idxs.size();
}

Data_Tree IndexExprAST::GetDataTree(bool from_assignment) { 
    return Idxs[0].start->GetDataTree();
}
  


ExitCheckExprAST::ExitCheckExprAST() {}

ChannelExprAST::ChannelExprAST(Parser_Struct *parser_struct, Data_Tree data_type,
                               std::string Name, bool isSelf) {
  this->parser_struct = parser_struct;
  this->data_type = data_type;
  this->Name = Name;
  this->isSelf = isSelf;
}

SpawnExprAST::SpawnExprAST(std::vector<std::unique_ptr<ExprAST>> Body, Parser_Struct *parser_struct) : Body(std::move(Body)) {
  this->parser_struct = parser_struct;
}


AsyncFnPriorExprAST::AsyncFnPriorExprAST() {}
  
void AsyncExprAST::Checks() {
  std::string async_scope = parser_struct->function_name + "_async";
  for (auto pair : typeVars[parser_struct->function_name])
    typeVars[async_scope][pair.first] = pair.second;
  for (auto pair : data_typeVars[parser_struct->function_name])
    data_typeVars[async_scope][pair.first] = pair.second;
}

void SpawnExprAST::Checks() {
  std::string async_scope = parser_struct->function_name + "_spawn";
  for (auto pair : typeVars[parser_struct->function_name])
    typeVars[async_scope][pair.first] = pair.second;
  for (auto pair : data_typeVars[parser_struct->function_name])
    data_typeVars[async_scope][pair.first] = pair.second;
}

void AsyncsExprAST::Checks() {
  std::string async_scope = parser_struct->function_name + "_asyncs";
  for (auto pair : typeVars[parser_struct->function_name])
    typeVars[async_scope][pair.first] = pair.second;
  for (auto pair : data_typeVars[parser_struct->function_name])
    data_typeVars[async_scope][pair.first] = pair.second;

  std::string type = this->Count->GetDataTree().Type;
  if (!in_vec(type, int_types))
    LogErrorC(parser_struct->line, "asyncs count must be int. Got " + type);
}

  /// AsyncExprAST - Expression class for async.
AsyncExprAST::AsyncExprAST(std::vector<std::unique_ptr<ExprAST>> Body, Parser_Struct *parser_struct)
  : Body(std::move(Body)) {
    this->parser_struct = parser_struct;
}
  
AsyncsExprAST::AsyncsExprAST(std::vector<std::unique_ptr<ExprAST>> Body, std::unique_ptr<ExprAST> Count, Parser_Struct *parser_struct)
  : Body(std::move(Body)), Count(std::move(Count)) {
    this->parser_struct = parser_struct;
}

IncThreadIdExprAST::IncThreadIdExprAST()
{}



Data_Tree SplitParallelExprAST::GetDataTree(bool from_assignment) {
  return Inner_Vec->GetDataTree();
}

SplitParallelExprAST::SplitParallelExprAST(std::unique_ptr<ExprAST> Inner_Vec) : Inner_Vec(std::move(Inner_Vec)) {
}

Data_Tree SplitStridedParallelExprAST::GetDataTree(bool from_assignment) {
  return Inner_Vec->GetDataTree();
}

SplitStridedParallelExprAST::SplitStridedParallelExprAST(std::unique_ptr<ExprAST> Inner_Vec) : Inner_Vec(std::move(Inner_Vec)) {
}
  
  /// FinishExprAST - Expression class for finish/async.
FinishExprAST::FinishExprAST(std::vector<std::unique_ptr<ExprAST>> Bodies,
              std::vector<bool> IsAsync)
        : Bodies(std::move(Bodies)), IsAsync(std::move(IsAsync)) {}
  
  
  
void LockExprAST::Checks() {
    for (auto &body : Bodies)
        body->Checks();
}
  /// LockExprAST
LockExprAST::LockExprAST(std::vector<std::unique_ptr<ExprAST>> Bodies,
            std::string Name)
        : Bodies(std::move(Bodies)), Name(Name) {}

  
  
  
  
MainExprAST::MainExprAST(std::vector<std::unique_ptr<ExprAST>> Bodies)
        : Bodies(std::move(Bodies)) {}
  
  

TemplateAST::TemplateAST(Parser_Struct *parser_struct,
        const std::string &Name, Data_Tree ReturnType,
        std::vector<std::string> Args,
        std::vector<Data_Tree> Types, bool is_op) 
    : parser_struct(parser_struct), Name(Name), ReturnType(ReturnType),
      Args(std::move(Args)), Types(std::move(Types)), IsOperator(is_op){
    
    CArgs = CallArgsTy(this->Types);
    CArgs.template_ast = this;
    CArgs.template_ret = ReturnType;
    CArgs.args = this->Args;
    CArgs.is_op = is_op;
    FnTemplates[Name].push_back(CArgs);
}

  
void PrototypeAST::SetDefaultArgs(std::vector<std::unique_ptr<ExprAST>> Inits) {
    if(Inits.size()==0)
        return;
    int inits = Inits.size();
    int i=Args.size()-inits;
    for (auto &expr : Inits) {
        std::string arg_name = this->Args[i++];
        ArgsInit[Name].emplace(arg_name, std::move(expr));
    }

    // Function_Arg_Count[this->Name] += inits;
    Function_Required_Arg_Count[this->Name] -= inits;


    // Handle positional args
    for (int i=0; i<inits; ++i) {
        auto dts = Types;
        dts.erase(dts.end()-inits, dts.end()-inits+i+1);
        CallArgsTy CArgs_variation = CallArgsTy(dts);
        AddFnVersion(BaseName, CArgs_variation, version);
    }
}
  
PrototypeAST::PrototypeAST(Parser_Struct *parser_struct,
              const std::string &Name,
              Data_Tree ReturnType, const std::string &Class,
              const std::string &Method,
              std::vector<std::string> Args,
              std::vector<Data_Tree> Types,
              bool IsOperator, unsigned Prec, bool overwrite)
      : Name(Name), ReturnType(ReturnType),
        Class(Class), Method(Method),
        IsOperator(IsOperator), Args(std::move(Args)), Types(std::move(Types)),
        Precedence(Prec) {
    this->parser_struct = parser_struct;

    BaseName = this->Name;
    bool has_generic=false;

    int ctx_offset = (parser_struct->gpu>0) ? 0 : 1;

    // Get Proto version name

    std::vector<Data_Tree> cargs_types = this->Types;
    // if (Class!="")
    //     cargs_types.insert(cargs_types.begin(), Data_Tree(Class));
    CArgs = CallArgsTy(cargs_types);
    if (IsOperator) {
        if (this->Types.size()==ctx_offset)
            LogErrorS(parser_struct->line, "Operator function prototype has no args.");
        if (this->Types.size()>ctx_offset+1)
            this->Name = this->Types[ctx_offset].Type +"_" + this->Types[ctx_offset+1].Type + "_" + this->Name;
        else
            this->Name = this->Types[ctx_offset].Type +"_" + this->Name;
    }

    if (Class!=""&&begins_with(BaseName, Class))
        ClassNativeMethods[Class].push_back(BaseName);
    



    // if (begins_with(this->Name, "layout")) {
    //     std::cout << "----SET VERSION FOR " << this->Name << "\n";
    //     std::cout << "generic " << is_generic << "\n";
    // }
    if (!IsOperator)
        version = SetFnVersion(this->Name, CArgs, overwrite);
    if (version!=0)
        this->Name += "_"+std::to_string(version);


    std::vector<std::string> arg_names;
    int arg_count=0; 
    for (auto arg : this->Types) {
        std::string arg_name = this->Args[arg_count++];
        Function_Arg_DataTypes[this->Name][arg_name] = arg;
        arg_names.push_back(arg_name);
        data_typeVars[this->Name][arg_name] = arg;
        typeVars[this->Name][arg_name] = arg.Type;
        CArgs.args.push_back(arg_name);

        if (arg.IsFromArena()) {
          int MemId = (*parser_struct->mem_id)++;
          fn_memid[this->Name][arg_name] = MemId;
          fn_arg_memid[this->Name][arg_name] = MemId;
        }
    }


    parser_struct->memid_arg_offset=(this->Name=="__anon_expr")
        ? 0 : *parser_struct->mem_id;


    fn_argnames[this->Name] = std::move(arg_names);
    int required_args = arg_count-ctx_offset;
    Function_Required_Arg_Count[this->Name] = required_args; // Desconsider scope_struct
    Function_Arg_Count[this->Name] = required_args;
    native_fn.push_back(this->Name);

    fn_ret_dt[this->Name] = ReturnType;
    CArgs.template_ret = ReturnType;


    if (ends_with(this->Name, "_prebuild")) {
        prebuild_functions.push_back(this->Name);
    }

    parser_struct->has_compiled_args = Fn_Compiled_Args.count(this->Name)>0;
}

const std::string &PrototypeAST::getName() const { return Name; }
const std::string &PrototypeAST::getClass() const { return Class; }
const std::string &PrototypeAST::getMethod() const { return Method; }





unsigned PrototypeAST::getBinaryPrecedence() const { return Precedence; }



Data_Tree ViewExprAST::GetDataTree(bool from_assignment) {
    return Data_Tree("str");
}
    
void ViewExprAST::Checks() {
    std::string R_Type = this->RHS->GetDataTree().Type;
    if(R_Type!="int") {
        if (!in_vec(R_Type, int_types))
            LogErrorS(parser_struct->line, "Tried to set view size as " + R_Type);
        else
            has_R_cast=true;
    }
}

ViewExprAST::ViewExprAST(std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS, Parser_Struct *parser_struct) \
            : LHS(std::move(LHS)), RHS(std::move(RHS)) {
    this->parser_struct = parser_struct;
}


Nameable *Nameable::InnerMost() {
  if (Inner->Depth==0)
    return this;
  return Inner->InnerMost(); 
}
Nameable *Nameable::Obj() {
  if (Inner->Depth==1)
    return this;
  return Inner->Obj(); 
}


std::string Nameable::GetLibCallee() {
  if (Inner->Depth==0)
    return Name;
  return Inner->GetLibCallee() + "__" + Name;
}


bool has_slice(std::vector<DimSlice> &indices) {
    bool has_slice = false;
    for (auto &idx : indices)
        has_slice |= idx.is_slice;
    return has_slice;
}

Data_Tree NameableIdx::GetLayoutDT(Data_Tree dt, bool from_assignment) {
    if (from_assignment||IsTile)
        return dt;

    auto dims = dt.FilterLayoutDims();

    if (Idx->Idxs.size()==dims.size())
        return dt.Nested_Data[0];

    return dt;
}

Data_Tree NameableIdx::GetDataTree(bool from_assignment) {

  Data_Tree inner_dt = Inner->GetDataTree();
  const std::string &compound_type = inner_dt.Type;

  if (in_vec(compound_type, {"charv","str"}))
      return Data_Tree("char");
 
  if(Idx_Fn_Return.count(compound_type+"_Idx")) {
    Data_Tree idx_data_tree = Data_Tree(Idx_Fn_Return[compound_type+"_Idx"]);
    return Data_Tree(Idx_Fn_Return[compound_type+"_Idx"]);
  }
  
  auto &indices = Idx->Idxs;



  if (!from_assignment&&(inner_dt.is_array||inner_dt.is_buffer))
      return Data_Tree(inner_dt.Type);

  
  if (from_assignment || has_slice(indices) ||\
          !in_vec(compound_type, compound_tokens))
    return inner_dt; 


  if(compound_type=="tuple") {
    if (IntExprAST *expr = dynamic_cast<IntExprAST*>(Idx->Idxs[0].start.get())) {

      int idx = expr->Val;
      if (idx>=inner_dt.Nested_Data.size())
        LogErrorS(parser_struct->line, "Tuple index out of range. Index at: " + std::to_string(idx) + ", but the tuple size is " + std::to_string(inner_dt.Nested_Data.size()));

      return Data_Tree(inner_dt.Nested_Data[idx].Type);
    } else
      LogErrorS(parser_struct->line, "Can only index tuple with a constant integer.");
  } 

  if(compound_type=="list" && inner_dt.Nested_Data.size()==0)
      return Data_Tree("any");

  if(compound_type=="map")
    return inner_dt.Nested_Data[1];

  
  return inner_dt.Nested_Data[0];
}


Data_Tree NameableLLVMIRCall::GetDataTree(bool from_assignment) {
  if (Callee=="pow"||Callee=="sqrt")
    return Data_Tree("float");

  return Data_Tree("unk");
}

NameableLLVMIRCall::NameableLLVMIRCall(Parser_Struct *parser_struct, std::unique_ptr<Nameable> Inner, std::vector<std::unique_ptr<ExprAST>> Args) : Nameable(parser_struct), Args(std::move(Args)) {
  this->Inner = std::move(Inner);
  this->Inner->IsLeaf = false;
  this->isSelf = this->Inner->isSelf;

  Depth = this->Inner->Depth;
  Callee = this->Inner->Name;
  // if (in_str(Callee, vararg_methods))
  // {
  //   if (Callee=="zip") {
  //     GetDataTree();
  //     this->Args.push_back(std::make_unique<NullPtrExprAST>());
  //   }
  //   else
  //     this->Args.push_back(std::make_unique<IntExprAST>(TERMINATE_VARARG));
  // }
}




Data_Tree NameableCall::GetDataTree(bool from_assignment) {
   Checks();



  if(is_tile) {
    Data_Tree dt = this->Inner->GetDataTree();
    return this->Inner->GetDataTree();
  }

  if(is_first_citizen) {
    data_type = this->Inner->GetDataTree().Nested_Data[0];
    return data_type;
  }

  Data_Tree ret = fn_ret_dt[Callee];

   

  std::string callee = (begins_with(Callee, "map_keys")) ? "map_keys" : Callee;
  callee = (begins_with(callee, "map_values")) ? "map_values" : callee;

  if (function_return_overwrite.count(callee)>0)
    ret = function_return_overwrite[callee](parser_struct, Args);
  if (method_return_overwrite.count(callee)>0)
    ret = method_return_overwrite[callee](parser_struct, Args, Inner);
  



  std::string ret_type = ret.Type;

  if(Callee=="map_keys") {
    Data_Tree return_dt = Data_Tree("array");
    return_dt.Nested_Data.push_back(Inner->GetDataTree().Nested_Data[0]);
    ret = return_dt;
  }
  if(Callee=="map_values") {
    Data_Tree return_dt = Data_Tree("array");
    return_dt.Nested_Data.push_back(Inner->GetDataTree().Nested_Data[1]);
    ret = return_dt;
  }
    

  if(Callee=="zip") {

    Data_Tree return_dt = Data_Tree("list");
    return_dt.Nested_Data.push_back(Data_Tree("list"));

    for(int i=0; i<Args.size(); ++i) {

      std::string type = Args[i]->GetDataTree().Nested_Data[0].Type;
      return_dt.Nested_Data[0].Nested_Data.push_back(Data_Tree(type));
    }

    ret = return_dt;
  }

  data_type = ret;
  ReturnType = ret.Type;


  return ret;
}



Data_Tree Nameable::GetDataTree(bool from_assignment) {  
  if(IsUnique) 
      return Data_Tree(Name);
  
  if(Depth==1) {
    if(Name=="self")
        data_type = Data_Tree(parser_struct->class_name);
    else if(in_vec(Name, primary_data_tokens))
        data_type = Data_Tree(Name);
    else if(data_typeVars[parser_struct->function_name].find(Name)!=data_typeVars[parser_struct->function_name].end()) {
        data_type = data_typeVars[parser_struct->function_name][Name];
        return data_type;
    }
    else if(fn_ret_dt.count(Name)>0||function_return_overwrite.count(Name)>0) {
        data_type = Data_Tree("function");
        return data_type;
    }
    else if (in_vec(Name, int_fn_values))
        data_type = Data_Tree("int");
    else if (IsPositionalArg(parser_struct, Name)) {
        data_type = Data_Tree("any");
        return data_type;
    } else if (parser_struct->cvalues.ints.count(Name)>0)
        return Data_Tree("int");
    else if (FnTemplates.count(Name)>0)
        return Data_Tree("generic_fn");
    else {
        bt(10);
        LogErrorS(Line, "Could not find variable " + Name + " on scope " + parser_struct->function_name + ".");
        data_type = Data_Tree("any"); // this allows to proceed with error checking
    }
    return data_type;
  }
  
  std::string scope = Inner->GetDataTree().Type;
  


  if(data_typeVars[scope].find(Name)!=data_typeVars[scope].end())
    data_type = data_typeVars[scope][Name];
  else if (Name=="tid"||Name=="tN")
      data_type = Data_Tree("int");
  else {
    LogErrorS(Line, "Could not find attribute " + Name + " on scope " + scope+". Depth: " + std::to_string(Depth));
    data_type = Data_Tree("any");
  }

  return data_type;
}





Nameable::Nameable(Parser_Struct *parser_struct) {
  this->parser_struct = parser_struct;
  this->Line = parser_struct->line;
}

Nameable::Nameable(Parser_Struct *parser_struct, std::string Name, int Depth) : Depth(Depth) {
  this->parser_struct = parser_struct;
  this->Name = Name;
  this->isAttribute = Depth>1;
  this->isSelf = (Depth==1&&Name=="self");
  this->Line = parser_struct->line;
}

Nameable::Nameable(Parser_Struct *parser_struct, std::string Name, int Depth, bool IsUnique) : Depth(Depth), IsUnique(IsUnique) {
  this->parser_struct = parser_struct;
  this->Name = Name;
  this->isAttribute = Depth>1;
  this->isSelf = (Depth==1&&Name=="self");
  this->Line = parser_struct->line;
  if (IsUnique && !in_vec(Name, Global_Uniques)) {
    Global_Uniques.push_back(Name);
    FunctionChecks(Name+"___init__");
  }
  if (Depth==1) {
      if (fn_memid[parser_struct->function_name].count(Name)>0)
          MemId = fn_memid[parser_struct->function_name][Name];
  }
}


NameableIdx::NameableIdx(Parser_Struct *parser_struct, std::unique_ptr<Nameable> Inner, std::unique_ptr<IndexExprAST> Idx, bool IsBracket) : Nameable(parser_struct), Idx(std::move(Idx)), IsBracket(IsBracket) {
  this->Inner = std::move(Inner); 
  this->Inner->IsLeaf = false;
  this->isSelf = this->Inner->isSelf;
}

void Nameable::AddNested(std::unique_ptr<Nameable> Inner) {
  this->Inner = std::move(Inner);
  this->Inner->IsLeaf = false;
  this->isSelf = this->isSelf||this->Inner->isSelf;
}

NameableRoot::NameableRoot(Parser_Struct *parser_struct) : Nameable(parser_struct) {
  Depth = 0;
  Name = "";
}


std::unique_ptr<ExprAST> Nameable::Copy() {
    return nullptr;
}


NameableCall::NameableCall(Parser_Struct *parser_struct, std::unique_ptr<Nameable> Inner, std::vector<std::unique_ptr<ExprAST>> Args, CallArgsTy CompiledArgsVec) : Nameable(parser_struct), Args(std::move(Args)), CompiledArgsVec(CompiledArgsVec) {
  this->Inner = std::move(Inner);
  this->Inner->IsLeaf = false;
  this->isSelf = this->Inner->isSelf;
  
  Depth = this->Inner->Depth;
  Callee = this->Inner->Name;
  
  if (Depth==1 && lib_function_remaps.count(Callee)>0)
    Callee = lib_function_remaps[Callee];

  if (function_own_ret_count.count(Callee)>0)
    OwnedId = (*parser_struct->owned_id)++;
  MemId = (*parser_struct->mem_id)++;
}


bool NameableCall::GetNeedGCSafePoint() {
    return true;
}

void Nameable::Checks() {
  if (checked)
    return;
  checked=true;
  if (IsUnique)
    FunctionChecks(Name+"___init__");
}


void NameableCall::Checks() {
  if (checked)
    return;
  checked=true;

  // methods/lib callee
  if(Depth>1) {    
    std::string inner_most_name = this->Inner->InnerMost()->Name;

    if (in_str(inner_most_name, imported_libs)) {
      FromLib=true; //example_lib.sum  
      Callee = this->Inner->GetLibCallee();
    }
    else {
      Data_Tree inner_dt = this->Inner->Inner->GetDataTree();
      if(data_typeVars[inner_dt.Type].find(Callee)!=data_typeVars[inner_dt.Type].end()) { // self.linear1(x)  
        Callee = UnmangleVec(data_typeVars[inner_dt.Type][Callee]);
      }
      else { // x.view()
        this->Inner = std::move(this->Inner->Inner);
        std::string inner_ty = UnmangleVec(inner_dt); 
        Callee = inner_ty + "_" + Callee;
      }
    } 
  }



  if (!in_vec(Callee, {"i8", "i64", "i16"})) {
      Data_Tree fdt = this->Inner->GetDataTree();
      is_first_citizen = fdt.Type=="Function";
  }

  // specify array type
  if(in_vec(Callee, {"array_shuffle", "array_print", "array_prod", "array_mean", "array_sum", "array_std"})) {
    std::string arrType = this->Inner->GetDataTree().Nested_Data[0].Type;
    if (!in_vec(arrType, primary_data_tokens)&&arrType!="str")
        arrType="void";
    Callee = Callee + "_" + arrType;
  }


  if(Callee=="map_keys") {
    std::string map_ty = this->Inner->GetDataTree().Nested_Data[0].Type;
    map_ty = (ClassSize.count(map_ty)>0) ? "void_ptr" : map_ty;
    Callee += "_" + map_ty;
  }
  if(Callee=="map_values") {
    std::string map_ty = this->Inner->GetDataTree().Nested_Data[1].Type;
    map_ty = (ClassSize.count(map_ty)>0) ? "void_ptr" : map_ty;
    Callee += "_" + map_ty;
  }
  if(Callee=="map_has") {
    std::string map_ty = this->Args[0]->GetDataTree().Type;
    map_ty = (ClassSize.count(map_ty)>0) ? "void_ptr" : map_ty;
    Callee += "_" + map_ty;
  }
  if(Callee=="map_get") {
    std::string value_ty = this->Inner->GetDataTree().Nested_Data[1].Type;
    if (!in_vec(value_ty,primary_data_tokens)&&!in_vec(value_ty,compound_tokens))
        value_ty = "any";
    Callee += "_" + this->Args[0]->GetDataTree().Type + "_" + value_ty;
  }


  // specify list type
  if(Callee=="list_append" && this->Args[0]->GetDataTree().Type=="int")
    Callee = "list_append_int";
  if(Callee=="list_append" && this->Args[0]->GetDataTree().Type=="float")
    Callee = "list_append_float";



    
  if (Callee=="array_append") {
      if (auto *idx_stmt = dynamic_cast<NameableIdx*>(this->Inner.get()))
          idx_stmt->IsAppend=true;
  }


  if (this->Inner) {
    if (auto *idx_stmt = dynamic_cast<NameableIdx*>(this->Inner.get())) {
        Data_Tree inner_dt = this->Inner->GetDataTree(true);
        is_tile=inner_dt.IsBuffered();
        idx_stmt->IsTile=is_tile;
        if (is_tile) {
            int args_size = Args.size();
            int idxs_size = idx_stmt->Idx->Idxs.size();
            if (args_size!=idxs_size)
                LogErrorS(parser_struct->line, "Tile size requires idxs equal to the number of tile");
            if (!idx_stmt->IsBracket&&inner_dt.Type!="layout")
                LogErrorS(parser_struct->line, "Type " + inner_dt.Type + " works only with bracket tiling.");

        }
    }
  }


  // vararg
  if (in_vec(Callee, vararg_methods)&&!in_vec(Callee, {"print", "printl"})) {
      std::string last_arg = fn_argnames[Callee][fn_argnames[Callee].size()-1];
      if (Function_Arg_Types[Callee][last_arg]=="int")
          this->Args.push_back(std::make_unique<IntExprAST>(TERMINATE_VARARG));
      else
          this->Args.push_back(std::make_unique<StringExprAST>("TERMINATE_VARARG"));
  }
 
  is_nsk_fn = in_str(Callee, native_methods);
  int sent_args = this->Args.size();
  bool is_native_method=false, is_method=false;
  if(Depth>1 && !FromLib) {
      sent_args++;
      std::string first_arg_dt = this->Inner->GetDataTree().Type; 
      // check is data method

      is_method = (begins_with(Callee, first_arg_dt) && !in_str(first_arg_dt, primary_data_tokens));        
      is_native_method = (ClassNativeMethods.count(first_arg_dt)>0&&in_vec(Callee, ClassNativeMethods[first_arg_dt]));
      is_nsk_fn = is_nsk_fn ||\
       (!is_native_method&&begins_with(Callee, first_arg_dt) && !in_str(first_arg_dt, primary_data_tokens));        
      // is_nsk_fn = is_nsk_fn ||\
       // (Classes.count(first_arg_dt)==0&&begins_with(Callee, first_arg_dt) && !in_str(first_arg_dt, primary_data_tokens));        
  }


  has_obj_overwrite = (Depth>1&&!FromLib&&!is_nsk_fn);

  bool is_obj = (Depth>1&&!FromLib&&is_nsk_fn); 

  if(is_obj)
      arg_type_check_offset++;
  if (gpu_fn.count(Callee)>0||gpu_ffi.count(Callee))
      arg_type_check_offset--;


  if (is_obj||(is_method&&!is_native_method))
      Types.push_back(Inner->GetDataTree());
  for (auto &arg : Args)
      Types.push_back(arg->GetDataTree());
  CArgs = CallArgsTy(Types);




  bool needs_version =\
        !(in_vec(Callee, vararg_methods)
        ||Callee=="Unnamed"
        ||data_typeVars[parser_struct->function_name].count(Callee)>0
        &&data_typeVars[parser_struct->function_name][Callee].Type=="Function");



  BaseCallee = Callee;
  if (needs_version) {
      
      Callee = SolveTemplate(parser_struct, Callee, CArgs);
      if (Callee!=BaseCallee && gpu_fn.count(BaseCallee)>0)
        gpu_fn[Callee] = 1;


      TemplateSolveCompiledArgs(Callee, BaseCallee);
  }
}



PositionalArgExprAST::PositionalArgExprAST(Parser_Struct *parser_struct, const std::string & ArgName, std::unique_ptr<ExprAST> Inner)
    : ArgName(ArgName), Inner(std::move(Inner)) {
  this->parser_struct = parser_struct;
}

Data_Tree PositionalArgExprAST::GetDataTree(bool from_assignment) {
    return Inner->GetDataTree(false);
}


void FunctionChecks(std::string fn_name) {


    if (TheJIT->fn_map.count(fn_name)>0) {
      if (in_vec(fn_name, fn_called))
           return;
      fn_called.push_back(fn_name);
      FunctionAST *fn = TheJIT->fn_map[fn_name];
      for (auto &body : fn->Body) {
        body->Traverse([](ExprAST *node) {
              node->Checks();
        });
      }
    }
}
