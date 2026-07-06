#pragma once

#include <string>
#include <vector>
#include "llvm/IR/Value.h"

#include "../runtime/data_types/data_tree.h"
#include "../runtime/compiler_frontend/parser_struct.h"

using namespace llvm;

struct CompiledArgs;
struct Arg_Pair;
class TemplateAST;
//===----------------------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------------------===//

/// ExprAST - Base class for all expression nodes.
class ExprAST {
  public:
    virtual ~ExprAST() = default;
    std::string Type = "None";
    std::string ReturnType = "None";
    std::string Name = "Unnamed";
    bool isSelf = false;
    bool isAttribute = false;
    std::string _pre_dot = "";
    bool isVec = false;
    bool isList = false;
    bool isVarLoad = false;
    bool SolverIncludeScope = true;
    bool NameSolveToLast = true;
    bool isMessage = false;
    int Line=-1;
    Parser_Struct *parser_struct;
  
    Data_Tree data_type = Data_Tree("");

  
  
    virtual Value *codegen(Value *scope_struct) = 0;
    virtual Data_Tree GetDataTree(bool from_assignment=false);

    virtual void SetType(std::string Type);
    virtual void SetReturnType(std::string ReturnType);
  
    virtual void SetIsVarLoad(bool isVarLoad);
    virtual bool GetIsVarLoad();
  
    virtual bool GetNameSolveToLast(); 
    virtual void SetNameSolveToLast(bool NameSolveToLast); 
  
    virtual void SetSelf(bool Self); 
    virtual bool GetSelf(); 
  
    virtual void SetSolverIncludeScope(bool SolverIncludeScope); 
    virtual bool GetSolverIncludeScope(); 
  
    virtual void SetIsAttribute(bool Attribute); 
    virtual bool GetIsAttribute(); 
    
  
    virtual void SetPreDot(std::string pre_dot); 
    virtual std::string GetPreDot(); 
  
    virtual std::string GetName(); 
    virtual void SetName(std::string Name); 
  
    
    virtual void SetIsVec(bool); 
    virtual bool GetIsVec(); 
  
    virtual void SetIsList(bool); 
    virtual bool GetIsList(); 

    virtual void SetIsMsg(bool); 
    virtual bool GetIsMsg(); 
    virtual void SetCValues(Parser_Struct *);

    virtual bool GetNeedGCSafePoint();
    // virtual nlohmann::json toJSON();
};


struct CallArgsTy {
    std::vector<Data_Tree> dts;
    bool has=false;
    std::vector<int8_t> i8s;
    std::vector<int16_t> i16s;
    std::vector<int> ints;
    std::vector<int64_t> i64s;
    std::vector<float> floats;
    std::vector<std::string> strings;
    std::vector<std::string> args;
    std::string version_str = "";
    int version = -1;
    TemplateAST *template_ast=nullptr;
    Data_Tree template_ret;

    CallArgsTy(std::vector<Data_Tree> dts);
    CallArgsTy(std::vector<std::unique_ptr<ExprAST>> *stmt=nullptr);
};

struct ArgsHasher {
    std::size_t operator()(const CallArgsTy& v) const {
        size_t hash = 0;
        
        for (auto &dt : v.dts)
            hash += dt.Hash();
        
        if (!v.has)
            return hash;
        return hash;
    }
};

struct ArgsEqual {
    bool operator()(const CallArgsTy& a,
                    const CallArgsTy& b) const {
        if (a.dts.size()!=b.dts.size()||a.has!=b.has)
            return false;
        for (int i=0;i<a.dts.size();++i) {
            if (a.dts[i].Compare(b.dts[i])>0)
                return false;
        }

        if (!a.has)
            return true;
        return false;
    }
};



struct DimSlice {
    std::unique_ptr<ExprAST> start, end;
    bool is_slice;
};
bool has_slice(std::vector<DimSlice> &indices);

class IndexExprAST : public ExprAST {
  public:
    int Size;
    std::vector<DimSlice> Idxs;
    std::string idx_slice_or_query = "idx";

    IndexExprAST(std::vector<DimSlice>);

    Value *codegen(Value *scope_struct) override;
    Data_Tree GetDataTree(bool from_assignment=false) override;
    int size() {
      return Size;
    }
}; 

  
  
/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {

  public:
    float Val;
    NumberExprAST(float Val); 

  Value *codegen(Value *scope_struct) override;
}; 
  
class IntExprAST : public ExprAST {
  public:
    int64_t Val;
    IntExprAST(int64_t Val); 
  Value *codegen(Value *scope_struct) override;
}; 


class BoolExprAST : public ExprAST {
  bool Val;
  public:
    BoolExprAST(bool Val); 
  Value *codegen(Value *scope_struct) override;
}; 

  
class StringExprAST : public ExprAST {
  public:
    std::string Val;
    StringExprAST(std::string Val); 

  Value *codegen(Value *scope_struct) override;
};

class CharExprAST : public ExprAST {
  public:
    int Val;
    CharExprAST(int Val); 

  Value *codegen(Value *scope_struct) override;
};
  


class NullPtrExprAST : public ExprAST {
  public:
    NullPtrExprAST(); 
  Value *codegen(Value *scope_struct) override;
};

 



  


 
class LutLoExprAST : public ExprAST {
  public:
    LutLoExprAST(); 
  Value *codegen(Value *scope_struct) override;
  Data_Tree GetDataTree(bool from_assignment=false) override;
}; 
class LutHiExprAST : public ExprAST {
  public:
    LutHiExprAST(); 
  Value *codegen(Value *scope_struct) override;
  Data_Tree GetDataTree(bool from_assignment=false) override;
}; 
  

/// VarExprAST - Expression class for var/in
class VarExprAST : public ExprAST {

  public:
    std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;
    
    std::string Type;
    VarExprAST(
        std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
        std::string Type);

  Value *codegen(Value *scope_struct) override;
};


class TupleExprAST : public VarExprAST {
  public:
    Data_Tree data_type;

    TupleExprAST(
      Parser_Struct *,
      std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
      std::string Type, Data_Tree data_type);

  Value *codegen(Value *scope_struct) override;
};

class ListExprAST : public VarExprAST {
  public:
    Data_Tree data_type;

    ListExprAST(
      Parser_Struct *,
      std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
      std::string Type, Data_Tree data_type);

  Value *codegen(Value *scope_struct) override;
};

class DictExprAST : public VarExprAST {
  public:
    Data_Tree data_type;

    DictExprAST(
      Parser_Struct *,
      std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
      std::string Type, Data_Tree data_type);

  Value *codegen(Value *scope_struct) override;
};



class UnkVarExprAST : public VarExprAST {
  public:
    std::vector<std::unique_ptr<ExprAST>> Notes;

    UnkVarExprAST(
      Parser_Struct *,
      std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
      std::string Type,
      std::vector<std::unique_ptr<ExprAST>> Notes);

  Value *codegen(Value *scope_struct) override;
  bool GetNeedGCSafePoint() override;
  void Checks();
};
  
  
  
class StrVecExprAST : public ExprAST {

  public:
    std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;
    std::string Type;
    
    StrVecExprAST(
        std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
        std::string Type);

  Value *codegen(Value *scope_struct) override;
};



class NewTupleExprAST : public ExprAST {

  public:
    std::vector<std::unique_ptr<ExprAST>> Values;
    Data_Tree data_type;
    
    NewTupleExprAST(
        std::vector<std::unique_ptr<ExprAST>> Values);

  Value *codegen(Value *scope_struct) override;
  Data_Tree GetDataTree(bool from_assignment=false) override;
};

class NewVecExprAST : public ExprAST {

  public:
    std::vector<std::unique_ptr<ExprAST>> Values;
    std::string Type;
    
    NewVecExprAST(
        std::vector<std::unique_ptr<ExprAST>> Values,
        std::string Type);

  Value *codegen(Value *scope_struct) override;
  Data_Tree GetDataTree(bool from_assignment=false) override;
  void Checks();
};

class NewDictExprAST : public ExprAST {

  public:
    std::vector<std::unique_ptr<ExprAST>> Keys, Values;
    std::string Type;
    
    NewDictExprAST(
        std::vector<std::unique_ptr<ExprAST>> Keys,
        std::vector<std::unique_ptr<ExprAST>> Values,
        std::string Type, Parser_Struct *);

  Value *codegen(Value *scope_struct) override;
};


class ObjectExprAST : public VarExprAST {

public:
  std::unique_ptr<ExprAST> Init;
  std::vector<bool> HasInit;
  std::vector<std::vector<std::unique_ptr<ExprAST>>> Args;
  std::string ClassName;

  ObjectExprAST(
      Parser_Struct *parser_struct,
      std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
      std::vector<bool> HasInit,
      std::vector<std::vector<std::unique_ptr<ExprAST>>> Args,
      std::string Type,
      std::unique_ptr<ExprAST> Init, std::string ClassName);

  Value *codegen(Value *scope_struct) override;
  void Checks();
};
  
  
   
  


  
  
class DataExprAST : public VarExprAST {
  public:
    std::vector<std::unique_ptr<ExprAST>> Notes;
    Data_Tree data_type;
    bool HasNotes, IsStruct, DtHasCreateFn;
    std::string dt_type, create_fn; 

    DataExprAST(
      Parser_Struct *,
      std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
      std::string Type, Data_Tree, bool, bool,
      std::vector<std::unique_ptr<ExprAST>> Notes);

  Value *codegen(Value *scope_struct) override;
  bool GetNeedGCSafePoint() override;
  void Checks();
};


class NewExprAST : public ExprAST {
  public:
    std::string DataName, Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;
    bool is_high_level_obj=false;
    Data_Tree data_type=Data_Tree("");

    NewExprAST(
      Parser_Struct *, std::string,
      std::vector<std::unique_ptr<ExprAST>> Args);

  Value *codegen(Value *scope_struct) override;
  Data_Tree GetDataTree(bool from_assignment=false) override;
  bool GetNeedGCSafePoint() override;
};


  
class LibImportExprAST : public ExprAST {
  public:
    std::string LibName;
    bool IsDefault;

  LibImportExprAST(std::string, bool, Parser_Struct *); 

  Value *codegen(Value *) override;
};
  
  
   
 
  
  
  
  
  


/// UnaryExprAST - Expression class for a unary operator.
class UnaryExprAST : public ExprAST {

public:
  int Opcode;
  std::unique_ptr<ExprAST> Operand;
  UnaryExprAST(int Opcode, std::unique_ptr<ExprAST> Operand, Parser_Struct *);

  Value *codegen(Value *scope_struct) override;
  bool GetNeedGCSafePoint() override;
  Data_Tree GetDataTree(bool from_assignment=false) override;
};
  
  
/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
  std::string cast_L_to="", cast_R_to="";
  Data_Tree L_dt, R_dt;

public:
  std::string Elements, Operation;
  bool is_store_sugar=false;
  std::unique_ptr<ExprAST> LHS, RHS;
  char Op;
  BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS, Parser_Struct *);

  Value *codegen(Value *scope_struct) override;
  Data_Tree GetDataTree(bool from_assignment=false) override;
  bool GetNeedGCSafePoint() override;
  void Checks();
};


class ConstExprAST : public ExprAST {
public:
  std::string str;
  Data_Tree data_type;
  
  ConstExprAST(Parser_Struct *, std::string);

  Value *codegen(Value *scope_struct) override;
  Data_Tree GetDataTree(bool from_assignment=false) override;
};



class ViewExprAST : public ExprAST {
  Data_Tree data_type;
public:
  std::unique_ptr<ExprAST> LHS;
  std::unique_ptr<ExprAST> RHS;
  bool has_R_cast=false;
  ViewExprAST(std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS, Parser_Struct *);
  Value *codegen(Value *scope_struct) override;
  Data_Tree GetDataTree(bool from_assignment=false) override;
  void Checks();
};







class Nameable : public ExprAST {
  public:
  std::vector<std::string> Expr_String = {};
  std::unique_ptr<Nameable> Inner=nullptr;
  int Depth=1;
  bool IsUnique=false,CanBeString=false,IsLeaf=true,Load_Last=true; 

  Nameable(Parser_Struct *);
  Nameable(Parser_Struct *, std::string, int);
  Nameable(Parser_Struct *, std::string, int, bool);

  void AddNested(std::unique_ptr<Nameable>);


  Value *codegen(Value *scope_struct) override;
  Data_Tree GetDataTree(bool from_assignment=false) override;
  Nameable *InnerMost();
  Nameable *Obj();

  std::string GetLibCallee();
  std::unique_ptr<ExprAST> Copy();
};


class NameableRoot : public Nameable {
  public:
  
  NameableRoot(Parser_Struct *);

  Value *codegen(Value *scope_struct) override;
};


class NameableLLVMIRCall : public Nameable {
  public:
  bool FromLib=false;
  std::vector<std::unique_ptr<ExprAST>> Args;
  std::string Callee, ReturnType="";

  NameableLLVMIRCall(Parser_Struct *, std::unique_ptr<Nameable> Inner, std::vector<std::unique_ptr<ExprAST>> Args);


  Value *codegen(Value *scope_struct) override;
  Data_Tree GetDataTree(bool from_assignment=false) override;
};


class NameableCall : public Nameable {
  bool checked=false;
  public:
  bool FromLib=false, is_nsk_fn=false, has_obj_overwrite, is_first_citizen=false;
  int arg_type_check_offset=1; 
  size_t hash=0;
  std::vector<std::unique_ptr<ExprAST>> Args;
  std::string Callee, ReturnType="";
  std::vector<Data_Tree> Types;
  CallArgsTy CompiledArgsVec;
  // FnCompiledValues CompiledArgs;
  CallArgsTy CArgs;

  NameableCall(Parser_Struct *, std::unique_ptr<Nameable> Inner, std::vector<std::unique_ptr<ExprAST>> Args, CallArgsTy);


  Value *codegen(Value *scope_struct) override;
  Value *codegen_append(Value *scope_struct);
  Data_Tree GetDataTree(bool from_assignment=false) override;
  bool GetNeedGCSafePoint() override;
  void Checks();
};


class NameableIdx : public Nameable {
  public:
  std::unique_ptr<IndexExprAST> Idx;

  NameableIdx(Parser_Struct *, std::unique_ptr<Nameable> Inner, std::unique_ptr<IndexExprAST> Idx);

  Value *codegen(Value *scope_struct) override;
  Data_Tree GetDataTree(bool from_assignment=false) override;
};







class PositionalArgExprAST : public ExprAST {
    public:
        std::string ArgName;
        std::unique_ptr<ExprAST> Inner;

        PositionalArgExprAST(Parser_Struct *, const std::string &, std::unique_ptr<ExprAST>);

    Value *codegen(Value *scope_struct) override;
    Data_Tree GetDataTree(bool from_assignment=false) override;
};










class NameableExprAST : public ExprAST {
  public:
  std::vector<std::string> Expr_String = {};
  std::unique_ptr<NameableExprAST> Inner_Expr;
  std::string Name="";
  bool End_of_Recursion=false, skip=false, IsLeaf=true, Load_Last=true, From_Self=false;
  int height=1;
  NameableExprAST();

  Value *codegen(Value *scope_struct) override;
};




class VariableListExprAST : public ExprAST {
    public:
        std::vector<std::unique_ptr<Nameable>> ExprList;
        VariableListExprAST(std::vector<std::unique_ptr<Nameable>> ExprList); 

    Value *codegen(Value *scope_struct) override;
    Data_Tree GetDataTree(bool from_assignment=false) override;
};

class EmptyStrExprAST : public NameableExprAST {
  
  public:
    EmptyStrExprAST();
    Value *codegen(Value *scope_struct) override;
};


class SelfExprAST : public NameableExprAST {
  
  public:
    SelfExprAST();
    Value *codegen(Value *scope_struct) override;
};


  

class NestedVectorIdxExprAST : public NameableExprAST {
  
  public:
    std::unique_ptr<IndexExprAST> Idx;
    NestedVectorIdxExprAST(std::unique_ptr<NameableExprAST>, std::string, Parser_Struct *, std::unique_ptr<IndexExprAST> Idx, std::string);
    Value *codegen(Value *scope_struct) override;

};






void Print_Names_Str(std::vector<std::string>);

class NestedVariableExprAST : public ExprAST {
  
  public:
    std::unique_ptr<NameableExprAST> Inner_Expr;
    bool Load_Val = true;
    
    NestedVariableExprAST(std::unique_ptr<NameableExprAST>, Parser_Struct *, std::string, Data_Tree);

    Value *codegen(Value *scope_struct) override;
    Data_Tree GetDataTree(bool from_assignment=false) override;
};

class NestedCallExprAST : public ExprAST {
  std::unique_ptr<NameableExprAST> Inner_Expr;
  std::string Callee;
  std::vector<std::unique_ptr<ExprAST>> Args;

  public:
    NestedCallExprAST(std::unique_ptr<NameableExprAST> Inner_Expr, std::string Callee, Parser_Struct *parser_struct,
                            std::vector<std::unique_ptr<ExprAST>> Args);
    Value *codegen(Value *scope_struct) override;
};
  
  

  

class RetExprAST : public ExprAST {

  public:
    std::vector<std::unique_ptr<ExprAST>> Vars;
    Data_Tree return_expected_type, returning_type;
    
    RetExprAST(std::vector<std::unique_ptr<ExprAST>> Vars, Parser_Struct *);

  Value *codegen(Value *scope_struct) override;
  void Checks();
};


struct fn_descriptor {
  std::string Name, Return;
  std::vector<std::string> ArgTypes, ArgNames;
  fn_descriptor(const std::string &, const std::string &);
};

class ClassExprAST : public ExprAST {
  public:
    std::string Name;
    std::vector<fn_descriptor> Functions;

    ClassExprAST(Parser_Struct *, const std::string &, const std::vector<fn_descriptor> &);

    Value *codegen(Value *scope_struct) override;
    
    // nlohmann::json toJSON() override;

    // Data_Tree GetDataTree(bool from_assignment=false) override;
};



class GCSafePointExprAST : public ExprAST {
  public:

  GCSafePointExprAST(Parser_Struct *); 

  Value *codegen(Value *) override;
};


/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Cond;

  public:
    std::vector<std::unique_ptr<ExprAST>> Then, Else;
    IfExprAST(Parser_Struct *,
              std::unique_ptr<ExprAST> Cond,
              std::vector<std::unique_ptr<ExprAST>> Then,
              std::vector<std::unique_ptr<ExprAST>> Else);

  Value *codegen(Value *scope_struct) override;
  Value *codegen_from_loop(Value *, BasicBlock *, BasicBlock *, BasicBlock *,
                std::map<std::string, Value*> &break_values_snapshot,
                std::vector<BasicBlock *> &BreakBB,
                std::vector<BasicBlock *> &ContinueBB);
};


  
/// ForExprAST - Expression class for for.
class ForExprAST : public ExprAST {
  std::string VarName;
  std::unique_ptr<ExprAST> Start, End, Step;

  public:
    std::vector<std::unique_ptr<ExprAST>> Body;
    ForExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Start,
              std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step,
              std::vector<std::unique_ptr<ExprAST>> Body, Parser_Struct *);

  Value *codegen(Value *scope_struct) override;
  void Checks();
  void SetCValues(Parser_Struct *) override;
};

/// ForExprAST - Expression class for for.
class ForEachExprAST : public ExprAST {
  std::string VarName;
  std::unique_ptr<ExprAST> Vec;

  public:
    std::vector<std::unique_ptr<ExprAST>> Body;
    ForEachExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Vec,
              std::vector<std::unique_ptr<ExprAST>> Body, Parser_Struct *);

  Value *codegen(Value *scope_struct) override;
  void Checks();
  void SetCValues(Parser_Struct *) override;
};

/// WhileExprAST - Expression class for while.
class WhileExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Cond;

  public:
    std::vector<std::unique_ptr<ExprAST>> Body;
    WhileExprAST(std::unique_ptr<ExprAST> Cond, std::vector<std::unique_ptr<ExprAST>> Body, Parser_Struct *);

  Value* codegen(Value *scope_struct) override;
  void SetCValues(Parser_Struct *) override;
};
  

class BreakExprAST : public ExprAST {
  public:
    BreakExprAST();
  Value *codegen(Value *scope_struct) override;
};
class ContinueExprAST : public ExprAST {
  public:
    ContinueExprAST();
  Value *codegen(Value *scope_struct) override;
};


class ExitCheckExprAST : public ExprAST {

  public:
    ExitCheckExprAST();

  Value* codegen(Value *scope_struct) override;
};


class ChannelExprAST : public ExprAST {

  public:
    ChannelExprAST(Parser_Struct *, Data_Tree, std::string, bool isSelf=false);

  Value* codegen(Value *scope_struct) override;
};



class AsyncFnPriorExprAST : public ExprAST {
  // std::string Async_Name;
  // std::vector<std::unique_ptr<ExprAST>> Body;

  public:
    AsyncFnPriorExprAST();
    // AsyncFnPriorExprAST(std::string, std::vector<std::unique_ptr<ExprAST>>, Parser_Struct *);

  Value* codegen(Value *scope_struct) override;
  void SetCValues(Parser_Struct *) override;
};

class SpawnExprAST : public ExprAST {
  std::vector<std::unique_ptr<ExprAST>> Body;

  public:
    SpawnExprAST(std::vector<std::unique_ptr<ExprAST>> Body, Parser_Struct *parser_struct);

  Value* codegen(Value *scope_struct) override;
  void Checks();
  void SetCValues(Parser_Struct *) override;
};




  
/// AsyncExprAST - Expression class for async.
class AsyncExprAST : public ExprAST {
  std::vector<std::unique_ptr<ExprAST>> Body;

  public:
    AsyncExprAST(std::vector<std::unique_ptr<ExprAST>> Body, Parser_Struct *parser_struct);

  Value* codegen(Value *scope_struct) override;
  void Checks();
  void SetCValues(Parser_Struct *) override;
};



/// FinishExprAST - Expression class for finish/async.
class FinishExprAST : public ExprAST {
  std::vector<std::unique_ptr<ExprAST>> Bodies;
  std::vector<bool> IsAsync;

  public:
    FinishExprAST(std::vector<std::unique_ptr<ExprAST>> Bodies,
                  std::vector<bool> IsAsync);


  Value* codegen(Value *scope_struct) override;
};


class AsyncsExprAST : public ExprAST {
  std::vector<std::unique_ptr<ExprAST>> Body;
  std::unique_ptr<ExprAST> Count;

  public:
    AsyncsExprAST(std::vector<std::unique_ptr<ExprAST>> Body, std::unique_ptr<ExprAST> Count, Parser_Struct *parser_struct);

  Value* codegen(Value *scope_struct) override;
  void Checks();
  void SetCValues(Parser_Struct *) override;
};
  


class IncThreadIdExprAST : public ExprAST {
  public:
    IncThreadIdExprAST();
  Value* codegen(Value *scope_struct) override;
};
  
  /// LockExprAST
class LockExprAST : public ExprAST {
  std::vector<std::unique_ptr<ExprAST>> Bodies;
  std::string Name;

  public:
    LockExprAST(std::vector<std::unique_ptr<ExprAST>> Bodies,
                std::string Name);


  Value* codegen(Value *scope_struct) override;
};


class ReduceExprAST : public ExprAST {

public:
  std::unique_ptr<ExprAST> LHS;
  std::string fn, functional_type;
  char Op;
  ReduceExprAST(Parser_Struct *, std::unique_ptr<ExprAST> LHS, char Op, std::string functional_type);

  Value *codegen(Value *scope_struct) override;
  Data_Tree GetDataTree(bool from_assignment=false) override;
  void Checks();
};

class LambdaExprAST : public ExprAST {

public:
  std::unique_ptr<ExprAST> Body;
  std::vector<std::string> Args;
  std::vector<Data_Tree> ArgsType;
  std::string lambda_fn;
  LambdaExprAST(Parser_Struct *, std::string lambda_fn, std::vector<std::string> Args);

  Value *codegen(Value *scope_struct) override;
  // Data_Tree GetDataTree(bool from_assignment=false) override;
};


class MapitExprAST : public ExprAST {

public:
  std::unique_ptr<LambdaExprAST> Lambda;
  std::unique_ptr<ExprAST> LHS;
  std::string fn="";
  MapitExprAST(Parser_Struct *, std::unique_ptr<ExprAST> LHS, std::unique_ptr<LambdaExprAST> Lambda);

  Value *codegen(Value *scope_struct) override;
  Data_Tree GetDataTree(bool from_assignment=false) override;
};




class LayoutExprAST : public ExprAST {

public:
  Data_Tree dt;
  uint16_t type;
  std::vector<std::unique_ptr<CompiledArgs>> CArgs;
  bool smem;
  std::vector<std::unique_ptr<ExprAST>> Args;

  LayoutExprAST(Parser_Struct *, uint16_t type, std::vector<std::unique_ptr<CompiledArgs>>, std::vector<std::unique_ptr<ExprAST>>, bool);

  Value *codegen(Value *scope_struct) override;
  Data_Tree GetDataTree(bool from_assignment=false) override;
  std::vector<Value *> GetStrides(Value *);
  int DimsProd();
};


class LaunchExprAST : public ExprAST {

public:
  std::unique_ptr<ExprAST> Grid, Block, Smem, Stream;
  std::vector<std::unique_ptr<ExprAST>> Args;
  CallArgsTy CompiledArgsVec;
  FnCompiledValues CompiledArgs;
  std::string fn_name;

  LaunchExprAST(Parser_Struct *, std::unique_ptr<ExprAST>, std::unique_ptr<ExprAST>, std::unique_ptr<ExprAST>, std::unique_ptr<ExprAST>, std::vector<std::unique_ptr<ExprAST>>, CallArgsTy CompiledArgsVec, std::string);

  Value *codegen(Value *scope_struct) override;
  void Checks();
};




class SplitParallelExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Inner_Vec;

  public:
    SplitParallelExprAST(std::unique_ptr<ExprAST> Inner_Vec);
    Value* codegen(Value *scope_struct) override;
    Data_Tree GetDataTree(bool from_assignment=false) override;
};

class SplitStridedParallelExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Inner_Vec;

  public:
    SplitStridedParallelExprAST(std::unique_ptr<ExprAST> Inner_Vec);
    Value* codegen(Value *scope_struct) override;
    Data_Tree GetDataTree(bool from_assignment=false) override;
};




class MainExprAST : public ExprAST {
  std::vector<std::unique_ptr<ExprAST>> Bodies;

  public:
    MainExprAST(std::vector<std::unique_ptr<ExprAST>> Bodies);


  Value* codegen(Value *scope_struct) override;
};


class TemplateAST {  
public:
    std::string BaseName, Name, Class, Method;
    int version=0;

    Parser_Struct *parser_struct;
    Data_Tree ReturnType;
    CallArgsTy CArgs;

    std::vector<std::string> Args;
    std::vector<Data_Tree> Types;

    TemplateAST(Parser_Struct *parser_struct, const std::string &Name, Data_Tree ReturnType,
          std::vector<std::string> Args,
          std::vector<Data_Tree> Types);
  
};

class PrototypeAST {  
    public:
    std::string BaseName, Name, Class, Method;
    int version=0;
  
    unsigned Precedence; // Precedence if a binary op.
  
      bool IsOperator=false, has_compiled_args=false, is_generic=false;
      Parser_Struct *parser_struct;
      Data_Tree ReturnType;
      CallArgsTy CArgs;
      std::vector<std::string> Args;
      std::vector<Data_Tree> Types;

      PrototypeAST(Parser_Struct *parser_struct, const std::string &Name, Data_Tree ReturnType,
                  const std::string &Class, const std::string &Method,
                  std::vector<std::string> Args,
                  std::vector<Data_Tree> Types,
                  bool IsOperator = false, unsigned Prec = 0, bool overwrite=false);

      PrototypeAST(Parser_Struct *,
                  const std::string &,
                  const std::string &,
                  CallArgsTy);
  
    Function *codegen(std::vector<std::unique_ptr<Arg_Pair>> *dynamic_args=nullptr);
    const std::string &getName() const; 
    const std::string &getClass() const; 
    const std::string &getMethod() const; 
  
    bool isUnaryOp() const; 
    bool isBinaryOp() const; 
    
    void SetDefaultArgs(std::vector<std::unique_ptr<ExprAST>> Inits);
  
    char getOperatorName() const; 
  
    unsigned getBinaryPrecedence() const; 
};


struct CompiledArgs {
    Data_Tree dt;
    std::string name;
    std::unique_ptr<ExprAST> expr;
    CompiledArgs(Data_Tree, std::string, std::unique_ptr<ExprAST> expr=nullptr);
};

struct Arg_Pair {
    Data_Tree dt;
    std::string name;
    std::unique_ptr<Nameable> expr;
    Arg_Pair(Data_Tree, std::string, std::unique_ptr<Nameable>);
};


int SetFnVersion(std::string fn, CallArgsTy CArgs, bool overwrite=false);
std::string GetFnVersion(Parser_Struct *parser_struct, std::string fn, CallArgsTy CArgs);

extern std::unordered_map<std::string,std::vector<std::unique_ptr<CompiledArgs>>> Fn_Compiled_Args;

extern std::unordered_map<std::string, std::vector<CallArgsTy>> FnVersion;
extern std::unordered_map<std::string, std::vector<CallArgsTy>> FnTemplates;
extern std::unordered_map<std::string,int> FnLastVersion;
