#pragma once

#include <string>
#include <vector>
#include "llvm/IR/Value.h"

#include "../runtime/data_types/data_tree.h"
#include "../runtime/compiler_frontend/parser_struct.h"

using namespace llvm;

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
  
    Data_Tree data_type = Data_Tree("");

  
  
    virtual Value *codegen(Value *scope_struct) = 0;
    virtual Data_Tree GetDataTree(bool from_assignment=false);

    virtual void SetType(std::string Type);
    virtual std::string GetType(bool from_assignment=false);
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

    virtual bool GetNeedGCSafePoint();

    // virtual nlohmann::json toJSON();
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
    Parser_Struct parser_struct;
    Data_Tree data_type;

    TupleExprAST(
      Parser_Struct,
      std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
      std::string Type, Data_Tree data_type);

  Value *codegen(Value *scope_struct) override;
};

class ListExprAST : public VarExprAST {
  public:
    Parser_Struct parser_struct;
    Data_Tree data_type;

    ListExprAST(
      Parser_Struct,
      std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
      std::string Type, Data_Tree data_type);

  Value *codegen(Value *scope_struct) override;
};

class DictExprAST : public VarExprAST {
  public:
    Parser_Struct parser_struct;
    Data_Tree data_type;

    DictExprAST(
      Parser_Struct,
      std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
      std::string Type, Data_Tree data_type);

  Value *codegen(Value *scope_struct) override;
};



class UnkVarExprAST : public VarExprAST {
  public:
    std::vector<std::unique_ptr<ExprAST>> Notes;
    Parser_Struct parser_struct;

    UnkVarExprAST(
      Parser_Struct,
      std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
      std::string Type,
      std::vector<std::unique_ptr<ExprAST>> Notes);

  Value *codegen(Value *scope_struct) override;
  bool GetNeedGCSafePoint() override;
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
};

class NewDictExprAST : public ExprAST {

  public:
    Parser_Struct parser_struct;
    std::vector<std::unique_ptr<ExprAST>> Keys, Values;
    std::string Type;
    
    NewDictExprAST(
        std::vector<std::unique_ptr<ExprAST>> Keys,
        std::vector<std::unique_ptr<ExprAST>> Values,
        std::string Type, Parser_Struct);

  Value *codegen(Value *scope_struct) override;
};


class ObjectExprAST : public VarExprAST {

public:
  Parser_Struct parser_struct;
  std::unique_ptr<ExprAST> Init;
  std::vector<bool> HasInit;
  std::vector<std::vector<std::unique_ptr<ExprAST>>> Args;
  std::string ClassName;

  ObjectExprAST(
      Parser_Struct parser_struct,
      std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
      std::vector<bool> HasInit,
      std::vector<std::vector<std::unique_ptr<ExprAST>>> Args,
      std::string Type,
      std::unique_ptr<ExprAST> Init, std::string ClassName);

  Value *codegen(Value *scope_struct) override;
};
  
  
   
  


  
  
class DataExprAST : public VarExprAST {
  public:
    std::vector<std::unique_ptr<ExprAST>> Notes;
    Data_Tree data_type;
    Parser_Struct parser_struct;
    bool HasNotes, IsStruct, DtHasCreateFn;
    std::string dt_type, create_fn; 

    DataExprAST(
      Parser_Struct,
      std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
      std::string Type, Data_Tree, bool, bool,
      std::vector<std::unique_ptr<ExprAST>> Notes);

  Value *codegen(Value *scope_struct) override;
  bool GetNeedGCSafePoint() override;
};


class NewExprAST : public ExprAST {
  public:
    std::string DataName, Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;
    bool is_high_level_obj=false;
    Data_Tree data_type=Data_Tree("");
    Parser_Struct parser_struct;

    NewExprAST(
      Parser_Struct, std::string,
      std::vector<std::unique_ptr<ExprAST>> Args);

  Value *codegen(Value *scope_struct) override;
  Data_Tree GetDataTree(bool from_assignment=false) override;
  bool GetNeedGCSafePoint() override;
};


  
class LibImportExprAST : public ExprAST {
  public:
    std::string LibName;
    bool IsDefault;
    Parser_Struct parser_struct;

  LibImportExprAST(std::string, bool, Parser_Struct); 

  Value *codegen(Value *) override;
};
  
  
   
 
  
  
  
  
  


/// UnaryExprAST - Expression class for a unary operator.
class UnaryExprAST : public ExprAST {
  Parser_Struct parser_struct;

public:
  int Opcode;
  std::unique_ptr<ExprAST> Operand;
  UnaryExprAST(int Opcode, std::unique_ptr<ExprAST> Operand, Parser_Struct);

  Value *codegen(Value *scope_struct) override;
  bool GetNeedGCSafePoint() override;
  Data_Tree GetDataTree(bool from_assignment=false) override;
};
  
  
/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
  Parser_Struct parser_struct;
  std::string cast_L_to="", cast_R_to="";
  Data_Tree L_dt, R_dt;

public:
  std::string Elements, Operation;
  bool is_store_sugar=false;
  std::unique_ptr<ExprAST> LHS, RHS;
  char Op;
  BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS, Parser_Struct);

  Value *codegen(Value *scope_struct) override;
  std::string GetType(bool from_assignment=false) override;
  Data_Tree GetDataTree(bool from_assignment=false) override;
  bool GetNeedGCSafePoint() override;
};


class ConstExprAST : public ExprAST {
public:
  Parser_Struct parser_struct;
  std::string str;
  Data_Tree data_type;
  
  ConstExprAST(Parser_Struct, std::string);

  Value *codegen(Value *scope_struct) override;
  Data_Tree GetDataTree(bool from_assignment=false) override;
};



class ViewExprAST : public ExprAST {
  Parser_Struct parser_struct;
  Data_Tree data_type;
public:
  std::unique_ptr<ExprAST> LHS;
  std::unique_ptr<ExprAST> RHS;
  bool has_R_cast=false;
  ViewExprAST(std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS, Parser_Struct);
  Value *codegen(Value *scope_struct) override;
  Data_Tree GetDataTree(bool from_assignment=false) override;
};







class Nameable : public ExprAST {
  public:
  std::vector<std::string> Expr_String = {};
  std::unique_ptr<Nameable> Inner=nullptr;
  int Depth=1;
  bool IsUnique=false,CanBeString=false,IsLeaf=true,Load_Last=true; 
  Parser_Struct parser_struct;

  Nameable(Parser_Struct);
  Nameable(Parser_Struct, std::string, int);
  Nameable(Parser_Struct, std::string, int, bool);

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
  
  NameableRoot(Parser_Struct);

  Value *codegen(Value *scope_struct) override;
};


class NameableLLVMIRCall : public Nameable {
  public:
  bool FromLib=false;
  std::vector<std::unique_ptr<ExprAST>> Args;
  std::string Callee, ReturnType="";

  NameableLLVMIRCall(Parser_Struct, std::unique_ptr<Nameable> Inner, std::vector<std::unique_ptr<ExprAST>> Args);


  Value *codegen(Value *scope_struct) override;
  Data_Tree GetDataTree(bool from_assignment=false) override;
};


class NameableCall : public Nameable {
  public:
  bool FromLib=false, is_nsk_fn=false, has_obj_overwrite, is_first_citizen;
  int arg_type_check_offset=1; 
  std::vector<std::unique_ptr<ExprAST>> Args;
  std::string Callee, ReturnType="";

  NameableCall(Parser_Struct, std::unique_ptr<Nameable> Inner, std::vector<std::unique_ptr<ExprAST>> Args);


  Value *codegen(Value *scope_struct) override;
  Data_Tree GetDataTree(bool from_assignment=false) override;
  bool GetNeedGCSafePoint() override;
};


class NameableIdx : public Nameable {
  public:
  std::unique_ptr<IndexExprAST> Idx;

  NameableIdx(Parser_Struct, std::unique_ptr<Nameable> Inner, std::unique_ptr<IndexExprAST> Idx);


  Value *codegen(Value *scope_struct) override;
  Data_Tree GetDataTree(bool from_assignment=false) override;
};


class NameableAppend : public Nameable {
  Data_Tree inner_dt;
  public:
  bool FromLib=false;
  std::vector<std::unique_ptr<ExprAST>> Args;
  std::string Callee, ReturnType="";

  NameableAppend(Parser_Struct, std::unique_ptr<Nameable> Inner, std::vector<std::unique_ptr<ExprAST>> Args);


  Value *codegen(Value *scope_struct) override;
  Data_Tree GetDataTree(bool from_assignment=false) override;
};





class PositionalArgExprAST : public ExprAST {
    public:
        std::string ArgName;
        std::unique_ptr<ExprAST> Inner;
        Parser_Struct parser_struct;

        PositionalArgExprAST(Parser_Struct, const std::string &, std::unique_ptr<ExprAST>);

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


class NestedStrExprAST : public NameableExprAST {
  Parser_Struct parser_struct;
  
  public:
    NestedStrExprAST(std::unique_ptr<NameableExprAST>, std::string, Parser_Struct);
    Value *codegen(Value *scope_struct) override;
};
  

class NestedVectorIdxExprAST : public NameableExprAST {
  Parser_Struct parser_struct;
  
  public:
    std::unique_ptr<IndexExprAST> Idx;
    NestedVectorIdxExprAST(std::unique_ptr<NameableExprAST>, std::string, Parser_Struct, std::unique_ptr<IndexExprAST> Idx, std::string);
    Value *codegen(Value *scope_struct) override;

    std::string GetType(bool from_assignment=false) override;
};






void Print_Names_Str(std::vector<std::string>);

class NestedVariableExprAST : public ExprAST {
  
  public:
    Parser_Struct parser_struct;
    std::unique_ptr<NameableExprAST> Inner_Expr;
    bool Load_Val = true;
    
    NestedVariableExprAST(std::unique_ptr<NameableExprAST>, Parser_Struct, std::string, Data_Tree);

    Value *codegen(Value *scope_struct) override;
    Data_Tree GetDataTree(bool from_assignment=false) override;
};

class NestedCallExprAST : public ExprAST {
  std::unique_ptr<NameableExprAST> Inner_Expr;
  std::string Callee;
  std::vector<std::unique_ptr<ExprAST>> Args;
  Parser_Struct parser_struct;

  public:
    NestedCallExprAST(std::unique_ptr<NameableExprAST> Inner_Expr, std::string Callee, Parser_Struct parser_struct,
                            std::vector<std::unique_ptr<ExprAST>> Args);
    Value *codegen(Value *scope_struct) override;
};
  
  

  

class RetExprAST : public ExprAST {

  public:
    std::vector<std::unique_ptr<ExprAST>> Vars;
    Parser_Struct parser_struct;
    Data_Tree return_expected_type, returning_type;
    
    RetExprAST(std::vector<std::unique_ptr<ExprAST>> Vars, Parser_Struct);

  Value *codegen(Value *scope_struct) override;
};


struct fn_descriptor {
  std::string Name, Return;
  std::vector<std::string> ArgTypes, ArgNames;
  fn_descriptor(const std::string &, const std::string &);
};

class ClassExprAST : public ExprAST {
  public:
    std::string Name;
    Parser_Struct parser_struct;
    std::vector<fn_descriptor> Functions;

    ClassExprAST(Parser_Struct, const std::string &, const std::vector<fn_descriptor> &);

    Value *codegen(Value *scope_struct) override;
    
    // nlohmann::json toJSON() override;

    // std::string GetType(bool from_assignment=false) override; 
    // Data_Tree GetDataTree(bool from_assignment=false) override;
};



class GCSafePointExprAST : public ExprAST {
  public:
    Parser_Struct parser_struct;

  GCSafePointExprAST(Parser_Struct); 

  Value *codegen(Value *) override;
};


/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Cond;
  Parser_Struct parser_struct;

  public:
    std::vector<std::unique_ptr<ExprAST>> Then, Else;
    IfExprAST(Parser_Struct,
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
  Parser_Struct parser_struct;

  public:
    std::vector<std::unique_ptr<ExprAST>> Body;
    ForExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Start,
              std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step,
              std::vector<std::unique_ptr<ExprAST>> Body, Parser_Struct);

  Value *codegen(Value *scope_struct) override;
};

/// ForExprAST - Expression class for for.
class ForEachExprAST : public ExprAST {
  std::string VarName;
  std::unique_ptr<ExprAST> Vec;
  Parser_Struct parser_struct;

  public:
    std::vector<std::unique_ptr<ExprAST>> Body;
    ForEachExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Vec,
              std::vector<std::unique_ptr<ExprAST>> Body, Parser_Struct, Data_Tree);

  Value *codegen(Value *scope_struct) override;
};

/// WhileExprAST - Expression class for while.
class WhileExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Cond;
  Parser_Struct parser_struct;

  public:
    std::vector<std::unique_ptr<ExprAST>> Body;
    WhileExprAST(std::unique_ptr<ExprAST> Cond, std::vector<std::unique_ptr<ExprAST>> Body, Parser_Struct);

  Value* codegen(Value *scope_struct) override;
};
  

class BreakExprAST : public ExprAST {
  Parser_Struct parser_struct;
  public:
    BreakExprAST();
  Value *codegen(Value *scope_struct) override;
};
class ContinueExprAST : public ExprAST {
  Parser_Struct parser_struct;
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
  Parser_Struct parser_struct;

  public:
    ChannelExprAST(Parser_Struct, Data_Tree, std::string, bool isSelf=false);

  Value* codegen(Value *scope_struct) override;
};



class AsyncFnPriorExprAST : public ExprAST {
  // std::string Async_Name;
  // Parser_Struct parser_struct;
  // std::vector<std::unique_ptr<ExprAST>> Body;

  public:
    AsyncFnPriorExprAST();
    // AsyncFnPriorExprAST(std::string, std::vector<std::unique_ptr<ExprAST>>, Parser_Struct);

  Value* codegen(Value *scope_struct) override;
};

class SpawnExprAST : public ExprAST {
  Parser_Struct parser_struct;
  std::vector<std::unique_ptr<ExprAST>> Body;

  public:
    SpawnExprAST(std::vector<std::unique_ptr<ExprAST>> Body, Parser_Struct parser_struct);

  Value* codegen(Value *scope_struct) override;
};




  
/// AsyncExprAST - Expression class for async.
class AsyncExprAST : public ExprAST {
  Parser_Struct parser_struct;
  std::vector<std::unique_ptr<ExprAST>> Body;

  public:
    AsyncExprAST(std::vector<std::unique_ptr<ExprAST>> Body, Parser_Struct parser_struct);

  Value* codegen(Value *scope_struct) override;
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
  Parser_Struct parser_struct;
  std::vector<std::unique_ptr<ExprAST>> Body;
  std::unique_ptr<ExprAST> Count;

  public:
    AsyncsExprAST(std::vector<std::unique_ptr<ExprAST>> Body, std::unique_ptr<ExprAST> Count, Parser_Struct parser_struct);

  Value* codegen(Value *scope_struct) override;
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


/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes), as well as if it is an operator.
class PrototypeAST {  
    std::string Name, Class, Method;
  
    bool IsOperator;
    unsigned Precedence; // Precedence if a binary op.
  
    public:
      Data_Tree ReturnType;
      std::vector<std::string> Args;
      std::vector<Data_Tree> Types;

      PrototypeAST(const std::string &Name, Data_Tree ReturnType,
                  const std::string &Class, const std::string &Method,
                  std::vector<std::string> Args,
                  std::vector<Data_Tree> Types,
                  bool IsOperator = false, unsigned Prec = 0);
  
    Function *codegen();
    const std::string &getName() const; 
    const std::string &getClass() const; 
    const std::string &getMethod() const; 
  
    bool isUnaryOp() const; 
    bool isBinaryOp() const; 
  
    char getOperatorName() const; 
  
    unsigned getBinaryPrecedence() const; 
};
