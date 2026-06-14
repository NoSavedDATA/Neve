

#include "llvm/IR/Value.h"


#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "include.h"
#include "../runtime/compiler_frontend/parser_struct.h"
#include "../runtime/data_types/data_tree.h"


using namespace llvm;
namespace fs = std::filesystem;



std::vector<std::string> imported_libs;
std::map<std::string, std::vector<std::string>> lib_submodules;


//===----------------------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------------------===//
void ExprAST::SetType(std::string Type) {
  this->Type=Type;
  this->ReturnType=Type;
}
std::string ExprAST::GetType(bool from_assignment) {
  return Type;
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

bool ExprAST::GetNeedGCSafePoint() {
    return false;
}


// nlohmann::json ExprAST::toJSON() {
//   nlohmann::json j;
//   return j;
// }
 









inline void Semantic_Arguments_Check(Parser_Struct parser_struct,
                                                  std::vector<std::unique_ptr<ExprAST>> &Args,
                                                  std::string fn_name,
                                                  bool is_nsk_fn, int sent_args, int arg_offset=1) {

  bool is_vararg = in_vec(fn_name, vararg_methods);

  if (Function_Required_Arg_Count.count(fn_name)>0) {
      if (sent_args<Function_Required_Arg_Count[fn_name] && !is_vararg)
          LogErrorS(parser_struct.line, "Passed " + std::to_string(sent_args) + " arguments to " + fn_name + ", but " + std::to_string(Function_Required_Arg_Count[fn_name]) + " are required.");
  }

  // -- Required Arguments -- //
  unsigned i, e;
  for (i = 0, e = Args.size(); i != e; ++i) {
    if (dynamic_cast<PositionalArgExprAST*>(Args[i].get()))
        break;
    
    Data_Tree data_type = Args[i]->GetDataTree();
    
    int tgt_arg = i + arg_offset;


    if(Function_Arg_Names.count(fn_name)==0) {
        LogErrorS(parser_struct.line, "Function " + fn_name + " does not require arguments.");
        return;
    }
    if(tgt_arg>=Function_Arg_Names[fn_name].size()) {
        LogErrorS(parser_struct.line, "Extrapolated " + fn_name + " arguments count.");
        return;
    }


    if (!in_vec(fn_name, {"to_int", "to_bool",  "to_float", "printl", "print"})) { 
      if (Function_Arg_DataTypes.count(fn_name)>0) {   
        Data_Tree expected_data_type = Function_Arg_DataTypes[fn_name][Function_Arg_Names[fn_name][tgt_arg]];


        int differences = expected_data_type.Compare(data_type);
        if (differences>0) { 
          LogErrorS(parser_struct.line, "Got an incorrect type for argument " + Function_Arg_Names[fn_name][tgt_arg] + " of function " + fn_name + ".");
          std::cout << "Expected\n   ";
          expected_data_type.Print();
          std::cout << "\nPassed\n   ";
          data_type.Print();
          std::cout << "\n\n";
        } 
      }
    }
  }


  i = i + arg_offset-1;
  // -- Add Default Arguments -- //
  if (Function_Arg_Count.count(fn_name)>0&&!is_vararg) {    
      for (; i<Args.size(); ++i) { // Positional Arguments
          auto PosArg = dynamic_cast<PositionalArgExprAST*>(Args[i].get());
          if(!PosArg)
            LogErrorS(parser_struct.line, "Standard argument followed by positional argument.");
      }
  }
}




  
 
  
  /// NumberExprAST - Expression class for numeric literals like "1.0".
NumberExprAST::NumberExprAST(float Val) : Val(Val) {
  this->SetType("float");
} 

IntExprAST::IntExprAST(int64_t Val) : Val(Val) {
  this->SetType("int");
} 

Data_Tree ConstExprAST::GetDataTree(bool from_assignment) {
    return Data_Tree("int");
}

ConstExprAST::ConstExprAST(Parser_Struct parser_struct, std::string str) : parser_struct(parser_struct), str(str) {
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


  
NewVecExprAST::NewVecExprAST(
    std::vector<std::unique_ptr<ExprAST>> Values,
    std::string Type)
    : Values(std::move(Values)), Type(Type) 
{
  this->SetType(Type);
  GetDataTree();
}


NewDictExprAST::NewDictExprAST(
    std::vector<std::unique_ptr<ExprAST>> Keys,
    std::vector<std::unique_ptr<ExprAST>> Values,
    std::string Type, Parser_Struct parser_struct)
    : Keys(std::move(Keys)), Values(std::move(Values)), Type(Type), parser_struct(parser_struct)
{
  this->SetType(Type);
}
  
  

ObjectExprAST::ObjectExprAST(
    Parser_Struct parser_struct,
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
  std::vector<bool> HasInit,
  std::vector<std::vector<std::unique_ptr<ExprAST>>> Args,
  std::string Type,
  std::unique_ptr<ExprAST> Init, std::string ClassName)
  : parser_struct(parser_struct), HasInit(std::move(HasInit)), Args(std::move(Args)), VarExprAST(std::move(VarNames), std::move(Type)), Init(std::move(Init)), ClassName(ClassName)
{

    for (unsigned i = 0, e = this->VarNames.size(); i != e; ++i)
    {
        if (this->HasInit[i]) { // callee init
          Semantic_Arguments_Check(parser_struct, this->Args[i], ClassName+"___init__", false, this->Args[i].size(), 1);
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
NestedStrExprAST::NestedStrExprAST(std::unique_ptr<NameableExprAST> Inner_Expr, std::string name, Parser_Struct parser_struct) : parser_struct(parser_struct)
                                    {
  this->Inner_Expr = std::move(Inner_Expr);
  this->Inner_Expr->IsLeaf=false;
  Name = name;
  
  From_Self = this->Inner_Expr->From_Self;
  height=this->Inner_Expr->height+1;

  Expr_String = this->Inner_Expr->Expr_String;
  Expr_String.push_back(name);
}

NestedVectorIdxExprAST::NestedVectorIdxExprAST(std::unique_ptr<NameableExprAST> Inner_Expr, std::string name, Parser_Struct parser_struct, std::unique_ptr<IndexExprAST> Idx, std::string type)
                                        : parser_struct(parser_struct), Idx(std::move(Idx)) {
  this->Inner_Expr = std::move(Inner_Expr);
  this->Inner_Expr->IsLeaf=false;
  this->Name = name;
  this->SetType(type);
  
  height=this->Inner_Expr->height+1;

  Expr_String = this->Inner_Expr->Expr_String;
  Expr_String.push_back(name);
  // Print_Names_Str(Expr_String);
}



NestedCallExprAST::NestedCallExprAST(std::unique_ptr<NameableExprAST> Inner_Expr, std::string Callee, Parser_Struct parser_struct,
  std::vector<std::unique_ptr<ExprAST>> Args)
  : Inner_Expr(std::move(Inner_Expr)), Callee(Callee), parser_struct(parser_struct), Args(std::move(Args)) {
}


NestedVariableExprAST::NestedVariableExprAST(std::unique_ptr<NameableExprAST> Inner_Expr, Parser_Struct parser_struct, std::string type, Data_Tree data_type)
      : Inner_Expr(std::move(Inner_Expr)), parser_struct(parser_struct) {
  this->SetType(type);

  this->Name = this->Inner_Expr->Name;
  this->data_type = data_type;
}
 
UnkVarExprAST::UnkVarExprAST(
  Parser_Struct parser_struct,
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
  std::string Type,
  std::vector<std::unique_ptr<ExprAST>> Notes)
  : parser_struct(parser_struct), VarExprAST(std::move(VarNames), std::move(Type)),
                Notes(std::move(Notes)) {

  for (unsigned i = 0, e = this->VarNames.size(); i != e; ++i) {
    const std::string &VarName = this->VarNames[i].first; 
    ExprAST *Init = this->VarNames[i].second.get();

    Data_Tree dt = Init->GetDataTree();

    if(Init->GetIsMsg()) {
      if(dt.Nested_Data.size()==0)
        LogBlue("Failed to receive message from " + Init->GetName() + ". Is it a channel?");
      dt = dt.Nested_Data[0];
    }
        
    if (Object_toClass[parser_struct.function_name].count(VarName)>0\
      ||data_typeVars[parser_struct.function_name].count(VarName)>0) {
        LogErrorS(parser_struct.line, "Redefinition of " + VarName);
        continue;
    }

    data_typeVars[parser_struct.function_name][VarName] = dt;
  }
}

bool UnkVarExprAST::GetNeedGCSafePoint() {
    return true;
}


TupleExprAST::TupleExprAST(
  Parser_Struct parser_struct,
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
  std::string Type,
  Data_Tree data_type) : parser_struct(parser_struct), VarExprAST(std::move(VarNames), std::move(Type)), data_type(data_type) {

    
  for (unsigned i = 0, e = this->VarNames.size(); i != e; ++i) {
    const std::string &VarName = this->VarNames[i].first; 
    ExprAST *Init = this->VarNames[i].second.get();

    std::string init_type = Init->GetType();
    Data_Tree other_type = Init->GetDataTree();
    
    if(this->Type=="tuple")
      Check_Is_Compatible_Data_Type(data_type, other_type, parser_struct);
    
    data_typeVars[parser_struct.function_name][VarName] = data_type;
  }
}

ListExprAST::ListExprAST(
  Parser_Struct parser_struct,
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
  std::string Type,
  Data_Tree data_type) : parser_struct(parser_struct), VarExprAST(std::move(VarNames), std::move(Type)), data_type(data_type) {

    
  for (unsigned i = 0, e = this->VarNames.size(); i != e; ++i) {
    const std::string &VarName = this->VarNames[i].first; 
    ExprAST *Init = this->VarNames[i].second.get();

    std::string init_type = Init->GetType();
    Data_Tree other_type = Init->GetDataTree();
    
    Check_Is_Compatible_Data_Type(data_type, other_type, parser_struct);
    
    data_typeVars[parser_struct.function_name][VarName] = data_type;
  }
}

DictExprAST::DictExprAST(
  Parser_Struct parser_struct,
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
  std::string Type,
  Data_Tree data_type) : parser_struct(parser_struct), VarExprAST(std::move(VarNames), std::move(Type)), data_type(data_type) {

    
  for (unsigned i = 0, e = this->VarNames.size(); i != e; ++i) {
    const std::string &VarName = this->VarNames[i].first; 
    ExprAST *Init = this->VarNames[i].second.get();

    std::string init_type = Init->GetType();
    Data_Tree other_type = Init->GetDataTree();
    
    Check_Is_Compatible_Data_Type(data_type, other_type, parser_struct);
    
    data_typeVars[parser_struct.function_name][VarName] = data_type;
  }
}

  
DataExprAST::DataExprAST(
  Parser_Struct parser_struct,
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
  std::string Type, Data_Tree data_type, bool HasNotes, bool IsStruct,
  std::vector<std::unique_ptr<ExprAST>> Notes)
  : parser_struct(parser_struct), VarExprAST(std::move(VarNames), std::move(Type)), data_type(data_type), HasNotes(HasNotes), IsStruct(IsStruct),
                Notes(std::move(Notes)) {   
  dt_type = "DT_"+data_type.Type;  

  if(data_type.Type=="charv") {      
      int size;
      if(this->Notes.size()<1)
            LogErrorS(parser_struct.line, "charv requires size argument");
      if (auto num_expr = dynamic_cast<IntExprAST*>(this->Notes[0].get()))
            size = num_expr->Val;
      else
            LogErrorS(parser_struct.line, "charv size must be int");

      data_type.Nested_Data.push_back(Data_Tree(std::to_string(size)));
  }

  for (unsigned i = 0, e = this->VarNames.size(); i != e; ++i) {
    if(this->isSelf)
      continue;    

    const std::string &VarName = this->VarNames[i].first;  
    ExprAST *Init = this->VarNames[i].second.get();

    Data_Tree init_dt = Init->GetDataTree();
    std::string init_type = init_dt.Type;

    Check_Is_Compatible_Data_Type(data_type, init_dt, parser_struct);

    if (!HasNotes&&\
        (Object_toClass[parser_struct.function_name].count(VarName)>0||\
         data_typeVars[parser_struct.function_name].count(VarName)>0)) {
        LogErrorS(parser_struct.line, "Redefinition of " +  VarName);
        continue;
    }

    data_typeVars[parser_struct.function_name][VarName] = data_type;
    typeVars[parser_struct.function_name][IdentifierStr] = data_type.Type;


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




bool DataExprAST::GetNeedGCSafePoint() {
    return true;
}

Data_Tree NewExprAST::GetDataTree(bool from_assignment) {
    if(data_type.Type!="")
        return data_type;

    // High-level class
    if (functions_return_data_type.count(Callee)==0) {
        Callee = DataName + "___init__";
        if (Classes.count(DataName)==0)
            LogErrorS(parser_struct.line, "New not implemented for data type " + DataName);
        is_high_level_obj = true;
        data_type = Data_Tree(DataName);
        return data_type;
    }

    // Other data types
    Data_Tree new_dt = functions_return_data_type[Callee];
    data_type = new_dt;
    return new_dt;
}

NewExprAST::NewExprAST(Parser_Struct parser_struct, std::string DataName, std::vector<std::unique_ptr<ExprAST>> Args)
            : parser_struct(parser_struct), DataName(DataName), Args(std::move(Args)) {
    Callee = DataName + "_Create";
    GetDataTree();
}

bool NewExprAST::GetNeedGCSafePoint() {
    return true;
}

LibImportExprAST::LibImportExprAST(std::string LibName, bool IsDefault, Parser_Struct parser_struct)
  : LibName(LibName), IsDefault(IsDefault), parser_struct(parser_struct) {


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
      LogErrorS(parser_struct.line, "Failed to import library: " + LibName + ".\n\t    Could not find .nv or lib.so file.");
    else
      imported_libs.push_back(LibName);
  }
}

  
  
  
  
  
  
  
  
  
  
  
  
Data_Tree UnaryExprAST::GetDataTree(bool from_assignment) {
    if (Opcode=='!')
        return Data_Tree("bool");
    return Operand->GetDataTree();
}

  /// UnaryExprAST - Expression class for a unary operator.
UnaryExprAST::UnaryExprAST(int Opcode, std::unique_ptr<ExprAST> Operand, Parser_Struct parser_struct)
    : Opcode(Opcode), Operand(std::move(Operand)), parser_struct(parser_struct) {}
  
bool UnaryExprAST::GetNeedGCSafePoint() {
    return Operand->GetNeedGCSafePoint();
}
  

// --Deprecated
std::string BinaryExprAST::GetType(bool from_assignment) {
  std::string type = Type;
  if (type=="None")
  {
    std::string LType = LHS->GetDataTree().Type, RType = RHS->GetDataTree().Type;
    if ((LType=="list"||RType=="list") && Op!='=')
      LogErrorS(parser_struct.line, "Tuple elements type are unknown during parsing type. Please load the element into a static type variable first.");
    
    Elements = LType + "_" + RType;    
    
    std::string operation = op_map[Op];
    Operation = Elements + "_" + operation;
    

    std::string type;
    if (Operation=="int_int_div")
      type = "float";
    if (elements_type_return.count(Operation)>0)
    {
      type = elements_type_return[Operation];
      std::cout << "found " << type << " for " << Operation << ".\n";
    }
    if (elements_type_return.count(Elements)>0)
      type = elements_type_return[Elements];
    SetType(type);

  }
  return type;
}


Data_Tree BinaryExprAST::GetDataTree(bool from_assignment) {
  std::string operation = op_map[Op];
  L_dt = LHS->GetDataTree();
  R_dt = RHS->GetDataTree();


  std::string LType = UnmangleVec(L_dt), RType = UnmangleVec(R_dt);
  if(ends_with(LType, "channel"))
    LType = "channel";
  if(ends_with(RType, "channel"))
    RType = "channel";

  if ((LType=="list"||RType=="list") && Op!='=')
    LogErrorS(parser_struct.line, "Tuple elements type are unknown during parsing type. Please load the element into a static type variable first.");

  if (LType=="char")
      LType = "i8";
  if (RType=="char")
      RType = "i8";


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
    cast_R_to="int_to_float";
  }
  if (in_vec(LType, {"str", "charv"})) {
      if (RType!="int"&&in_vec(RType, int_types)) {
          cast_R_to="to_int";
          Elements = LType+"_int";
      }
  }
  // std::cout << Elements << " | " << Operation << "\n";
  Operation = Elements + "_" + operation;

  if (LType=="channel" && !in_str(RType, primary_data_tokens)&&RType!="str")
    Operation = "channel_void_message";

  // std::cout << "\n";
  //   std::cout << "op" << "\n";
  //   L_dt.Print();
  //   std::cout << Operation << "\n";

  if (RType=="channel" && !in_str(LType, primary_data_tokens)&&LType!="str")
    Operation = "void_channel_message";
  std::string type;
  if (Operation=="int_int_div")
    type = "float";
  else if (Elements=="vec_vec")
      return L_dt;
  else if (Elements=="vec_int")
      return L_dt;
  else if (Elements=="int_vec")
      return R_dt;
  else if (ops_type_return.count(Operation)>0)
    return Data_Tree(ops_type_return[Operation]);
  else if (functions_return_data_type.count(Operation)) {
    Data_Tree dt = functions_return_data_type[Operation];
    return functions_return_data_type[Operation];
  }
  else if (elements_type_return.count(Elements)>0)
    type = elements_type_return[Elements];
  else {
      if (Op!='=')
          LogErrorS(parser_struct.line, "Operation function " + Operation + " not found.");
  }

  return Data_Tree(type);
}

bool IsPositionalArg(Parser_Struct parser_struct, std::string name) {
    if (ArgsInit.count(parser_struct.parse_fn)>0) {
        if (ArgsInit[parser_struct.parse_fn].count(name)> 0)
            return true;
    }
    return false;
}

bool BinaryExprAST::GetNeedGCSafePoint() {
    return (LHS->GetNeedGCSafePoint()||RHS->GetNeedGCSafePoint());
}
  
  /// binaryexprAST - Expression class for a binary operator.
BinaryExprAST::BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
              std::unique_ptr<ExprAST> RHS, Parser_Struct parser_struct)
    : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)), parser_struct(parser_struct) {

  GetDataTree();

  std::string LType = L_dt.Type;
  std::string Lname = this->LHS->GetName();

  if (IsPositionalArg(parser_struct, Lname)) {
    return;
  }

  std::string RType = R_dt.Type;
  // std::cout << LType << "|" << RType << " -- " << Op << "\n";


  // --- Handle store --- //
  if (Op == '=' || (Op==tok_arrow&&!begins_with(Elements, "channel"))) {


    // ch <-
    if(Op==tok_arrow) {
      if(ChannelDirections[parser_struct.function_name].count(this->RHS->GetName())==0)
        LogErrorS(parser_struct.line, "Could not find channel " + this->RHS->GetName());
      if(ChannelDirections[parser_struct.function_name][this->RHS->GetName()]==ch_receiver)
        LogErrorS(parser_struct.line, "Trying to unpack data from a receiver only channel.");
      
      // todo: test this
      // if (!in_str(LType, primary_data_tokens) && !is_high_lvl_obj) {
      //     std::string copy_fn = LType + "_Copy";
      //     Function *F = TheModule->getFunction(copy_fn);
      //     if (!F) 
      //         return LogErrorV(parser_struct.line, "Tried to use channel operation for " + \
      //                                               LType + ", but this data type has no Copy implementation.");
      // }
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
                    LogErrorS(parser_struct.line, "Querying " + key_type + " map with " + LHSV->Idx->GetDataTree().Type);
        }

        // arr[x] = y
        if (LType=="array")
          Check_Is_Compatible_Data_Type(L_dt, R_dt, parser_struct);
    }

    // is_alloca
    else if(!this->LHS->GetSelf()&&!this->LHS->GetIsAttribute()) {
      Check_Is_Compatible_Data_Type(L_dt, R_dt, parser_struct);

      if (data_typeVars[parser_struct.function_name].count(Lname)==0)
          LogErrorS(parser_struct.line, "Variable " + Lname + " not yet declared");
      
    } else
        Check_Is_Compatible_Data_Type(L_dt, R_dt, parser_struct);

    return;
  }


  // --- Handle operation --- //
  
  CheckIsSenderChannel(Elements, parser_struct, Lname);

  if(L_dt.Type=="channel"||R_dt.Type=="channel")
    Check_Is_Compatible_Data_Type(L_dt, R_dt, parser_struct);
}
  
  
  





  
  
  
  
  
 
  






RetExprAST::RetExprAST(std::vector<std::unique_ptr<ExprAST>> Vars, Parser_Struct parser_struct)
    : Vars(std::move(Vars)), parser_struct(parser_struct) {

    return_expected_type = functions_return_data_type[parser_struct.function_name];

    if (this->Vars.size()==1) {
        returning_type = this->Vars[0]->GetDataTree();

        if (!Check_Is_Compatible_Data_Type(return_expected_type, returning_type, parser_struct))
            std::cout << "*Incompatible return type" << ".\n\n\n";
    }
}
    
  
fn_descriptor::fn_descriptor(const std::string &Name, const std::string &Return) : Name(Name), Return(Return) {}

ClassExprAST::ClassExprAST(Parser_Struct parser_struct, const std::string &Name, const std::vector<fn_descriptor> &Functions)
  : Name(Name), parser_struct(parser_struct), Functions(Functions) {}

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
  
GCSafePointExprAST::GCSafePointExprAST(Parser_Struct parser_struct) : parser_struct(parser_struct) {}
  
  
  /// IfExprAST - Expression class for if/then/else.
IfExprAST::IfExprAST(Parser_Struct parser_struct,
          std::unique_ptr<ExprAST> Cond,
          std::vector<std::unique_ptr<ExprAST>> Then,
          std::vector<std::unique_ptr<ExprAST>> Else)
    : parser_struct(parser_struct), Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}
  
  
/// ForExprAST - Expression class for for.
ForExprAST::ForExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Start,
          std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step,
          std::vector<std::unique_ptr<ExprAST>> Body, Parser_Struct parser_struct)
    : VarName(VarName), Start(std::move(Start)), End(std::move(End)),
      Step(std::move(Step)), Body(std::move(Body)), parser_struct(parser_struct) {}
  

/// ForExprAST - Expression class for for.
ForEachExprAST::ForEachExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Vec,
          std::vector<std::unique_ptr<ExprAST>> Body, Parser_Struct parser_struct, Data_Tree data_type)
    : VarName(VarName), Vec(std::move(Vec)), Body(std::move(Body)), parser_struct(parser_struct) {

    this->data_type = data_type;
    Type = data_type.Nested_Data[0].Type;
    typeVars[parser_struct.function_name][VarName] = "foreach_control_var";
}


  
  /// WhileExprAST - Expression class for while.
WhileExprAST::WhileExprAST(std::unique_ptr<ExprAST> Cond, std::vector<std::unique_ptr<ExprAST>> Body, Parser_Struct parser_struct)
  : Cond(std::move(Cond)), Body(std::move(Body)), parser_struct(parser_struct) {}

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

ChannelExprAST::ChannelExprAST(Parser_Struct parser_struct, Data_Tree data_type, std::string Name, bool isSelf) : parser_struct(parser_struct) {
  this->data_type = data_type;
  this->Name = Name;
  this->isSelf = isSelf;
  std::cout << "--ChannelExpr" << "\n";
}

SpawnExprAST::SpawnExprAST(std::vector<std::unique_ptr<ExprAST>> Body, Parser_Struct parser_struct) : Body(std::move(Body)), parser_struct(parser_struct) {}

// AsyncFnPriorExprAST::AsyncFnPriorExprAST(std::string Async_Name, std::vector<std::unique_ptr<ExprAST>> Body, Parser_Struct parser_struct) : Async_Name(Async_Name), Body(std::move(Body)), parser_struct(parser_struct) {}
AsyncFnPriorExprAST::AsyncFnPriorExprAST() {}
  
  /// AsyncExprAST - Expression class for async.
AsyncExprAST::AsyncExprAST(std::vector<std::unique_ptr<ExprAST>> Body, Parser_Struct parser_struct)
  : Body(std::move(Body)), parser_struct(parser_struct) {}
  
AsyncsExprAST::AsyncsExprAST(std::vector<std::unique_ptr<ExprAST>> Body, int AsyncsCount, Parser_Struct parser_struct)
  : Body(std::move(Body)), AsyncsCount(AsyncsCount), parser_struct(parser_struct) {}

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
  
  
  
  
  /// LockExprAST
LockExprAST::LockExprAST(std::vector<std::unique_ptr<ExprAST>> Bodies,
            std::string Name)
        : Bodies(std::move(Bodies)), Name(Name) {}

  
  
  
  
MainExprAST::MainExprAST(std::vector<std::unique_ptr<ExprAST>> Bodies)
        : Bodies(std::move(Bodies)) {}
  
  
  
  
PrototypeAST::PrototypeAST(const std::string &Name, Data_Tree ReturnType, const std::string &Class,
              const std::string &Method,
              std::vector<std::string> Args,
              std::vector<Data_Tree> Types,
              bool IsOperator, unsigned Prec)
      : Name(Name), ReturnType(ReturnType), Class(Class), Method(Method), Args(std::move(Args)), Types(std::move(Types)),
        IsOperator(IsOperator), Precedence(Prec) {

    functions_return_data_type[Name] = ReturnType;
    // std::cout << Name << "|" << this->Types.size() << "|" << this->Args.size() << "\n";

    std::vector<std::string> arg_names;
    int arg_count=0; 
    for (auto arg : this->Types) {
        std::string arg_name = this->Args[arg_count++];
        Function_Arg_DataTypes[Name][arg_name] = arg;
        // std::cout << "proto e " << Name << " add " << arg_name << "\n";
        arg_names.push_back(arg_name);
        data_typeVars[Name][arg_name] = arg;
    }

    int initialized_args = (ArgsInit.count(Name)>0) ? ArgsInit[Name].size() : 0;
    
    Function_Arg_Names[Name] = std::move(arg_names);
    Function_Required_Arg_Count[Name] = arg_count-1-initialized_args; // Desconsider scope_struct
    native_fn.push_back(Name);

    if (ends_with(Name, "_prebuild"))
        prebuild_functions.push_back(Name);
    // std::cout << "proto " << Name << " has " << arg_count-1 << " args\n";
}

const std::string &PrototypeAST::getName() const { return Name; }
const std::string &PrototypeAST::getClass() const { return Class; }
const std::string &PrototypeAST::getMethod() const { return Method; }

bool PrototypeAST::isUnaryOp() const { return IsOperator && Args.size() == 1; }
bool PrototypeAST::isBinaryOp() const { return IsOperator && Args.size() == 2; }

char PrototypeAST::getOperatorName() const {
  assert(isUnaryOp() || isBinaryOp());
  return Name[Name.size() - 1];
}



unsigned PrototypeAST::getBinaryPrecedence() const { return Precedence; }



Data_Tree ViewExprAST::GetDataTree(bool from_assignment) {
    return Data_Tree("str");
}
    
ViewExprAST::ViewExprAST(std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS, Parser_Struct parser_struct) \
            : LHS(std::move(LHS)), RHS(std::move(RHS)), parser_struct(parser_struct) {
    std::string R_Type = this->RHS->GetDataTree().Type;
    if(R_Type!="int") {
        if (!in_vec(R_Type, int_types))
            LogErrorS(parser_struct.line, "Tried to set view size as " + R_Type);
        else
            has_R_cast=true;
    }
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
  
  if (from_assignment || has_slice(indices) ||\
          !in_vec(compound_type, compound_tokens))
    return inner_dt; //e.g: for list_Store_Idx


  if(compound_type=="tuple") {
    if (IntExprAST *expr = dynamic_cast<IntExprAST*>(Idx->Idxs[0].start.get())) {

      int idx = expr->Val;
      if (idx>=inner_dt.Nested_Data.size())
        LogErrorS(parser_struct.line, "Tuple index out of range. Index at: " + std::to_string(idx) + ", but the tuple size is " + std::to_string(inner_dt.Nested_Data.size()));

      return Data_Tree(inner_dt.Nested_Data[idx].Type);
    } else
      LogErrorS(parser_struct.line, "Can only index tuple with a constant integer.");
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

NameableLLVMIRCall::NameableLLVMIRCall(Parser_Struct parser_struct, std::unique_ptr<Nameable> Inner, std::vector<std::unique_ptr<ExprAST>> Args) : Nameable(parser_struct), Args(std::move(Args)) {
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

Data_Tree NameableAppend::GetDataTree(bool from_assignment) {  
  if (data_type.Type!="")
    return data_type;

  Data_Tree ret = functions_return_data_type[Callee];
   

  std::string ret_type = ret.Type;
  if (ends_with(ret_type, "_vec")) {
    Data_Tree return_dt = Data_Tree("vec");
    return_dt.Nested_Data.push_back(remove_suffix(ret_type, "_vec"));
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



Data_Tree NameableCall::GetDataTree(bool from_assignment) {

  if (data_type.Type!="")
    return data_type;

  if(is_first_citizen) {
    data_type = this->Inner->GetDataTree().Nested_Data[0];
    return data_type;
  }

  Data_Tree ret = functions_return_data_type[Callee];


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
    // std::cout << "Get dt of " << Name << "\n";
    if (data_type.Type!="")
        return data_type;

  if(IsUnique)
      return Data_Tree(Name);
  
  if(Depth==1) {
    if(Name=="self")
        data_type = Data_Tree(parser_struct.class_name);
    else if(in_vec(Name, primary_data_tokens))
        data_type = Data_Tree(Name);
    else if(data_typeVars[parser_struct.function_name].find(Name)!=data_typeVars[parser_struct.function_name].end())
        data_type = data_typeVars[parser_struct.function_name][Name];
    else if(functions_return_data_type.count(Name)>0||function_return_overwrite.count(Name)>0) {
        data_type = Data_Tree("function");
        return data_type;
    }
    else if (Name=="tid"||Name=="tN")
        data_type = Data_Tree("int");
    else if (IsPositionalArg(parser_struct, Name)) {
        data_type = Data_Tree("any");
        return data_type;
    } else {


        LogErrorS(parser_struct.line, "Could not find variable " + Name + " on scope " + parser_struct.function_name + ".");
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
    LogErrorS(parser_struct.line, "Could not find variable " + Name + " on scope " + scope+". Depth: " + std::to_string(Depth));
    data_type = Data_Tree("any");
  }
  return data_type;
}



Nameable::Nameable(Parser_Struct parser_struct) : parser_struct(parser_struct) {}

Nameable::Nameable(Parser_Struct parser_struct, std::string Name, int Depth) : parser_struct(parser_struct), Depth(Depth) {
  this->Name = Name;
  this->isAttribute = Depth>1;
  this->isSelf = (Depth==1&&Name=="self");
}

Nameable::Nameable(Parser_Struct parser_struct, std::string Name, int Depth, bool IsUnique) : parser_struct(parser_struct), Depth(Depth), IsUnique(IsUnique) {
  this->Name = Name;
  this->isAttribute = Depth>1;
  this->isSelf = (Depth==1&&Name=="self");
  if (IsUnique && !in_vec(Name, Global_Uniques))
      Global_Uniques.push_back(Name);
}


NameableIdx::NameableIdx(Parser_Struct parser_struct, std::unique_ptr<Nameable> Inner, std::unique_ptr<IndexExprAST> Idx) : Nameable(parser_struct), Idx(std::move(Idx)) {
  this->Inner = std::move(Inner); 
  this->Inner->IsLeaf = false;
  this->isSelf = this->Inner->isSelf;
}

void Nameable::AddNested(std::unique_ptr<Nameable> Inner) {
  this->Inner = std::move(Inner);
  this->Inner->IsLeaf = false;
  this->isSelf = this->isSelf||this->Inner->isSelf;
}

NameableRoot::NameableRoot(Parser_Struct parser_struct) : Nameable(parser_struct) {
  Depth = 0;
  Name = "";
}


std::unique_ptr<ExprAST> Nameable::Copy() {
    return nullptr;
}


NameableAppend::NameableAppend(Parser_Struct parser_struct, std::unique_ptr<Nameable> Inner, std::vector<std::unique_ptr<ExprAST>> Args)
                                : Nameable(parser_struct), Args(std::move(Args))
{
    AddNested(std::move(Inner));

    this->inner_dt = this->Inner->Inner->GetDataTree();
    Callee = this->inner_dt.Type + "_" + this->Inner->Name;
    
    this->Inner = std::move(this->Inner->Inner);


    if (this->inner_dt.Type=="array") {
        std::string appended_type = this->Args[0]->GetDataTree().Type;
        std::string elem_type = inner_dt.Nested_Data[0].Type;
        
        if(!(elem_type=="float"&&appended_type=="int") && appended_type!=elem_type&&\
                elem_type!="Function")
            LogErrorS(parser_struct.line, "Tried to append " + appended_type + " into a " + elem_type + " array.");
    }
}

NameableCall::NameableCall(Parser_Struct parser_struct, std::unique_ptr<Nameable> Inner, std::vector<std::unique_ptr<ExprAST>> Args) : Nameable(parser_struct), Args(std::move(Args)) {
  this->Inner = std::move(Inner);
  this->Inner->IsLeaf = false;
  this->isSelf = this->Inner->isSelf;

  
  Depth = this->Inner->Depth;
  Callee = this->Inner->Name;


  
  if (Depth==1 && lib_function_remaps.count(Callee)>0)
    Callee = lib_function_remaps[Callee];


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
        Callee = UnmangleVec(inner_dt) + "_" + Callee;
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


  if(Callee=="map_keys")
    Callee += "_" + this->Inner->GetDataTree().Nested_Data[0].Type;
  if(Callee=="map_values")
    Callee += "_" + this->Inner->GetDataTree().Nested_Data[1].Type;
  if(Callee=="map_has")
    Callee += "_" + this->Args[0]->GetDataTree().Type;
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



  // check if exists
  if (functions_return_data_type.count(Callee)==0&&function_return_overwrite.count(Callee)==0\
        &&method_return_overwrite.count(Callee)==0\
          &&!this->isSelf&&!is_first_citizen) {
    // std::cout << "" << Callee << "|" << std::to_string(!in_vec(Callee, template_fn)) << "\n";
    // std::cout << "" << this->isSelf << "\n";
      LogErrorS(parser_struct.line, "Function " + Callee + " not yet implemented.");
      return;
  }
    

  // vararg
  if (in_str(Callee, vararg_methods)) {
    if (Callee=="zip") {
      GetDataTree();
      this->Args.push_back(std::make_unique<NullPtrExprAST>());
    }
    else {
      std::string last_arg = Function_Arg_Names[Callee][Function_Arg_Names[Callee].size()-1];
      if (Function_Arg_Types[Callee][last_arg]=="int")
          this->Args.push_back(std::make_unique<IntExprAST>(TERMINATE_VARARG));
      else
          this->Args.push_back(std::make_unique<StringExprAST>("TERMINATE_VARARG"));
    }
  }
 
  is_nsk_fn = in_str(Callee, native_methods);
  int sent_args = this->Args.size();
  if(Depth>1 && !FromLib) {
      sent_args++;
      std::string first_arg_dt = this->Inner->GetDataTree().Type; 
      // check is data method
      is_nsk_fn = is_nsk_fn ||\
       (Classes.count(first_arg_dt)==0&&begins_with(Callee, first_arg_dt) && !in_str(first_arg_dt, primary_data_tokens));        
  }


  has_obj_overwrite = (Depth>1&&!FromLib&&!is_nsk_fn);

  if(Depth>1&&!FromLib&&is_nsk_fn)
      arg_type_check_offset++;


  if(!is_first_citizen)
      Semantic_Arguments_Check(parser_struct, this->Args, Callee, is_nsk_fn, sent_args, arg_type_check_offset);
}


bool NameableCall::GetNeedGCSafePoint() {
    return true;
}



PositionalArgExprAST::PositionalArgExprAST(Parser_Struct parser_struct, const std::string & ArgName, std::unique_ptr<ExprAST> Inner)
    : parser_struct(parser_struct), ArgName(ArgName), Inner(std::move(Inner)) {}

Data_Tree PositionalArgExprAST::GetDataTree(bool from_assignment) {
    return Inner->GetDataTree(false);
}
