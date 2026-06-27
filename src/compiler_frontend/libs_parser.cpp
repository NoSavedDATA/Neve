#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"

#include <cstdlib>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>



#include "../nsk_cpp.h"
#include "../libs_llvm/so_libs.h"
#include "include.h"


std::map<std::string, std::string> lib_function_remaps;
std::map<std::string, Data_Tree> Idx_Fn_Return;

std::vector<std::unique_ptr<ExprAST>> fn_arg_inits;



using namespace llvm;
namespace fs = std::filesystem;

std::vector<fs::path> glob_cpp(const fs::path& rootDir, std::string extension=".cpp") {
    std::vector<fs::path> cppFiles;

    for (const auto& entry : fs::recursive_directory_iterator(rootDir)) {
        const std::string filename = entry.path().string();
        if (filename.size() >= extension.size() &&
            filename.compare(filename.size() - extension.size(), extension.size(), extension) == 0) {
            cppFiles.push_back(entry.path());
        }
    }
    return std::move(cppFiles);
}

inline std::vector<fs::path> get_lib_files(std::string lib_dir)
{
  std::vector<fs::path> files = glob_cpp(lib_dir);
  std::vector<fs::path> cu_files = glob_cpp(lib_dir, ".cu");

  files.insert(files.end(), cu_files.begin(), cu_files.end());

  return files;
}

inline void recognize_data_type(std::string type) {
    std::string nsk_type = remove_substring(type, "DT_");

    if(!in_str(nsk_type, data_tokens))
        data_tokens.push_back(nsk_type);

    if (data_name_to_type().count(nsk_type)==0) {
        data_name_to_type()[nsk_type] = data_type_count;
        data_type_to_name()[data_type_count++] = nsk_type;
    }
}


LibFunction::LibFunction(std::string ReturnType, bool IsPointer, std::string Name, std::vector<std::string> ArgTypes, std::vector<std::string> ArgNames, std::vector<int> ArgIsPointer, bool IsVarArg, bool HasRetOverwrite, std::string LibType, Data_Tree LibDT, int DefaultArgsCount)
    : ReturnType(ReturnType), IsPointer(IsPointer), Name(Name), ArgTypes(ArgTypes), ArgNames(ArgNames), ArgIsPointer(ArgIsPointer), IsVarArg(IsVarArg),
      HasRetOverwrite(HasRetOverwrite), LibType(LibType), LibDT(LibDT), DefaultArgsCount(DefaultArgsCount) {
}

void LibFunction::Print() {
    std::cout << "extern \"C\" " << ReturnType << " ";
    if(IsPointer)
      std::cout << "*";
    std::cout << Name << "(";

    for(int i=0;i<ArgTypes.size(); ++i)
    {
      std::cout << ArgTypes[i];
      if (ArgIsPointer[i])
        std::cout << " *"; 

      if(i<ArgTypes.size()-1)
      std::cout << ", ";
    }

    std::cout << ")\n\n";
}
void LibParser::PrintFunctions() {
    for (auto pair : Functions) {
        for (auto fn : pair.second)
            fn->Print();
    }
}


void LibFunction::Link_to_LLVM(void *func_ptr, void *handle) {
    if(!has_main)
        return;

    llvm::Type *int8PtrTy = Type::getInt8Ty(*TheContext)->getPointerTo();
    llvm::Type *floatTy = Type::getFloatTy(*TheContext);
    llvm::Type *halfTy = Type::getFloatTy(*TheContext);
    llvm::Type *boolTy = Type::getInt1Ty(*TheContext);
    llvm::Type *intTy = Type::getInt32Ty(*TheContext);

    llvm::Type *fn_return_type;
    std::vector<llvm::Type *> arg_types;

    std::string fn_return_type_str;
    std::vector<std::string> arg_types_str;


    if (HasRetOverwrite) {
        fn_return_type = int8PtrTy;
        fn_return_type_str = LibType;
    }
    else if(ReturnType=="int"&&!IsPointer) {
        fn_return_type = intTy;
        fn_return_type_str = "int";
    }
    else if(ReturnType=="float") {
        if (IsPointer) {
            ReturnType = "float_ptr";
            fn_return_type = int8PtrTy;
            fn_return_type_str = "float_ptr";
        } else {
            fn_return_type = floatTy;
            fn_return_type_str = "float";
        }
    }
    else if(ReturnType=="bool"&&!IsPointer) {
        fn_return_type = boolTy;
        fn_return_type_str = "bool";
    }
    else if(begins_with(ReturnType, "DT_")) {
        fn_return_type = int8PtrTy;
        recognize_data_type(ReturnType);
    }
    else {
        fn_return_type = int8PtrTy;
        fn_return_type_str = "void_ptr";
    }



    
    if (ends_with(Name, "_Create")) {
        std::string create_type = Name;
        create_type = remove_suffix(create_type, "_Create");

        std::string return_type = ReturnType;
        if (begins_with(return_type, "DT_"))
            return_type = remove_substring(return_type, "DT_");

        if (create_type!=return_type) {
            Equivalent_Types[create_type].push_back(return_type);

            int type_id = data_name_to_type()[return_type];
            data_name_to_type()[create_type] = type_id;
            // data_type_to_name[type_id] = equivalent_type;
        }
    }
    

    for(int i=0; i<ArgTypes.size(); ++i) {
        if (!begins_with(Name, "initialize__"))
        {
            std::string type = ArgTypes[i];

            if(begins_with(type, "DT_"))
                type = remove_substring(type, "DT_");
            if(type=="char"&&ArgIsPointer[i])
                type = "str";
            if(type=="std::vector<char*>"||type=="std::vector<char>")
            {
                LogBlue("It is vec char*");
                type = "str_vec";
            }

            Function_Arg_Names[Name].push_back(ArgNames[i]);
            Function_Arg_Types[Name][ArgNames[i]] = type;

            if(ends_with(type, "_vec")) {
                Data_Tree vec_dt = Data_Tree("vec");
                vec_dt.Nested_Data.push_back(Data_Tree(remove_suffix(type, "_vec")));
                Function_Arg_DataTypes[Name][ArgNames[i]] = vec_dt;
            } else 
                Function_Arg_DataTypes[Name][ArgNames[i]] = Data_Tree(type);
        }
        

        if(ArgTypes[i]=="int"&&!ArgIsPointer[i]) {
            arg_types.push_back(intTy);
            arg_types_str.push_back("int");
        }
        else if(ArgTypes[i]=="float"&&!ArgIsPointer[i]) {
            arg_types.push_back(floatTy);
            arg_types_str.push_back("float");
        }
        else {
            arg_types.push_back(int8PtrTy);
            arg_types_str.push_back("void_ptr");
        }
    }


    if (Name=="_glob_b_")
        fn_return_type_str = "str_vec";
    Lib_Functions_Return[Name] = fn_return_type_str; // for llvm return type (so_libs.cpp)
    Lib_Functions_Args[Name] = std::move(arg_types_str);
    Function_Arg_Count[Name] = ArgTypes.size()-1; // ignoring scope struct
    DefaultArgsCount++; // scope struct
    Function_Required_Arg_Count[Name] = ArgTypes.size()-DefaultArgsCount;


    Data_Tree return_dt;
    if (HasRetOverwrite)
        return_dt = LibDT;
    else
        return_dt = Data_Tree(fn_return_type_str);
    functions_return_data_type[Name] = return_dt;

    FunctionType *llvm_function = FunctionType::get(
        fn_return_type,
        arg_types,
        IsVarArg
    );
    Function* funcDecl = cast<Function>(
        TheModule->getOrInsertFunction(Name, llvm_function).getCallee()
    );

    auto &ES = TheJIT->JIT->getExecutionSession();

    // THIS replaces old Mangle(Name)
    auto Sym = ES.intern(Name);

    llvm::orc::SymbolMap symbols;

    symbols[Sym] = llvm::orc::ExecutorSymbolDef{
        llvm::orc::ExecutorAddr::fromPtr(func_ptr),
        llvm::JITSymbolFlags::Exported | llvm::JITSymbolFlags::Callable
    };

    if (auto Err = TheJIT->getMainJITDylib().define(
            llvm::orc::absoluteSymbols(symbols)))
        LogError(-1, "Failed to define native function in JIT: " + llvm::toString(std::move(Err)));

}


void LibFunction::Add_to_Nsk_Dicts(void *func_ptr, std::string lib_name, bool is_default) {
    user_cpp_functions.push_back(Name);
    native_methods.push_back(Name);
    native_functions.push_back(Name);
    native_fn.push_back(Name);
    if (IsVarArg)
        vararg_methods.push_back(Name);


    // Check if it is a data-type
    if (IsPointer||HasRetOverwrite)
    {
        std::string nsk_data_type = ReturnType;
        if(begins_with(ReturnType, "DT_"))
        { 
            nsk_data_type.erase(0, 3);
            recognize_data_type(nsk_data_type);
        }
        if (Name=="_glob_b_")
            nsk_data_type = "str_vec";

        if (HasRetOverwrite)
            functions_return_data_type[Name] = LibDT;
        else if(ends_with(nsk_data_type, "_vec")) {
            Data_Tree vec_type = Data_Tree("vec");
            vec_type.Nested_Data.push_back(Data_Tree(remove_substring(nsk_data_type, "_vec")));
            functions_return_data_type[Name] = vec_type;
        }
        else
            functions_return_data_type[Name] = Data_Tree(nsk_data_type);
        functions_return_type[Name] = nsk_data_type;
    }


    if (ends_with(Name, "_Idx")||ends_with(Name, "_Query")) {
        // LogBlue(Name + " is _Idx with return " + ReturnType + " and is pointer " + std::to_string(IsPointer));
        if(ReturnType=="char"&&IsPointer)
            Idx_Fn_Return[Name] = Data_Tree("str");
        else {
            std::string nsk_data_type = ReturnType;
            if(begins_with(ReturnType, "DT_"))
                nsk_data_type.erase(0, 3);
            if(ends_with(nsk_data_type, "_vec")) {
                Data_Tree vec_tree = Data_Tree("vec");
                vec_tree.Nested_Data.push_back(remove_substring(nsk_data_type, "_vec"));
                Idx_Fn_Return[Name] = vec_tree; 
            } else
                Idx_Fn_Return[Name] = Data_Tree(nsk_data_type);
        }
    }


    // Check for data-type operations return type
    for (std::string operation : op_map_names)
    {
        if (ends_with(Name, operation))
        {
            // std::cout << "OPERATION FUNCTION FOUND " << Name << ".\n\n\n\n\n";

            std::string operands = Name;

            size_t pos = operands.rfind(operation)-1;
            if (pos != std::string::npos) {
                operands.replace(pos, operation.length()+1, "");
            }


            std::string nsk_data_type = ReturnType;
            if(begins_with(nsk_data_type, "DT_"))
                nsk_data_type.erase(0, 3);

            elements_type_return.insert(std::make_pair(operands, nsk_data_type));

            break;
        }
    }
    

    // Check if it is a _Clean_Up or _backward function    
    if(ends_with(Name, "_Clean_Up")&&has_main)
    {
        // std::cout << "FOUND CLEAN UP FUNCTION " << Name << ".\n";

        std::string nsk_type = Name;
        size_t pos = nsk_type.rfind("_Clean_Up");
        if (pos != std::string::npos) {
            nsk_type.replace(pos, 9, "");
        }
        recognize_data_type(nsk_type);

        using CleanupFunc = void(*)(void*,int);
        CleanupFunc casted_func_ptr = reinterpret_cast<CleanupFunc>(func_ptr);
        clean_up_functions[nsk_type] = casted_func_ptr;

    } else if (ends_with(Name, "_backward")) {
        // std::cout << "FOUND BACKWARD FN" << ".\n";
    }    
    else {

    }


    // Create a remap when using "import default"
    if(is_default && begins_with(Name, lib_name)) {
        std::string remaped_fn = erase_before_pattern(Name, "__");
        lib_function_remaps[remaped_fn] = Name;
    }
}



LibParser::LibParser(std::string lib_dir) {
    files = get_lib_files(lib_dir);
}


char LibParser::_getCh() {

    if(!file.is_open()) {
        file_idx++;
        file.open(files[file_idx]);
        // std::cout << "Now parsing" << files[file_idx] << ".\n";
    }

    if (file.get(ch)) {
        return ch;
    } else {
        // std::cout << "Reached file ending" << ".\n";
        file.close();

        file_idx++;
        if (file_idx<files.size())
        file.open(files[file_idx]);

        return tok_eof;
    }
}


void LibParser::ParseDT(Data_Tree &dt) {
    LastChar = _getCh(); // eat <

    if (!(isalpha(LastChar)||LastChar=='_')) { // identifier
        LogError(-1, "Expected data type identifier after < of nested data tree inside $> expression ");
    }

    while(true) {
        std::string nested_type="";
        while(isalpha(LastChar)||LastChar=='_') { 
            nested_type += LastChar;
            LastChar = _getCh();
        }
        Data_Tree nested_dt = Data_Tree(nested_type);
        if (LastChar=='<')
            ParseDT(nested_dt);
        while (LastChar==32)
            LastChar = _getCh();
        if (LastChar==',')
            LastChar = _getCh();
        while (LastChar==32)
            LastChar = _getCh();
        dt.Nested_Data.push_back(nested_dt);
        if (LastChar=='>')
            break;
    }
    LastChar = _getCh(); // eat >
}


bool LibParser::TryParseFnDataType() {
    LastChar = _getCh(); // eat $
    if (LastChar!='>')
        return false;
    LastChar = _getCh(); // eat >

    while(LastChar==32||LastChar==tok_tab)
        LastChar = _getCh();

    if (!(isalpha(LastChar)||LastChar=='_')) { // identifier
        LogError(-1, "Expected data type identifier after $> expression ");
    }
    
    lib_type = "";
    while(isalpha(LastChar)||LastChar=='_') { 
        lib_type += LastChar;
        LastChar = _getCh();
    }


    lib_dt = Data_Tree(lib_type);

    if (LastChar=='<')
        ParseDT(lib_dt);

    
    while(LastChar==32||LastChar==tok_tab) 
        LastChar = _getCh();


    // Try parse arguments as well
    if (LastChar=='$') {
        LastChar=_getCh();
        Parser_Struct parser_struct;
        char PreCurTok = CurTok;

        
        tokenizer->has_lib_file=true;
        tokenizer->lib_file = std::move(file);
        CurTok = tok_identifier;

        getNextToken();
        if(CurTok==tok_space)
            getNextToken();
        


        tokenizer->can_see_space=false;
        while (CurTok==tok_identifier||CurTok==tok_data) {

            bool is_data = CurTok==tok_data;

            std::string arg_name = IdentifierStr;
            getNextToken(); // eat identifier

            if (CurTok=='='&&is_data) {
                LogError(-1, "(C++ Library Parse Error) Tried to set a data type as a default argument");
                std::exit(0);
            }
            else if (CurTok!='='&&!is_llvm) {
                LogError(-1, "(C++ Library Parse Error) Expected \"=\" for arg init.");
                std::exit(0);
            } else {
                LLVMFunction_Args.push_back(Data_Tree(arg_name));
                continue;
            }

            getNextToken(); // eat =

            auto arg = ParseExpression(parser_struct, "");
            ArgsInit[fn_name].emplace(arg_name, std::move(arg));
            CurDefaultArgs++;
        }
        tokenizer->can_see_space=true;
        tokenizer->has_lib_file=false;

        file = std::move(tokenizer->lib_file);
        CurTok = PreCurTok;
    }
    
    while(LastChar!=10 && LastChar!=tok_eof && LastChar!=tok_finish)
        LastChar = _getCh();


    return true;
}


int LibParser::_getTok() {
    bool consume_no_ret=false;

    while (LastChar==10||LastChar==13||LastChar==tok_space||LastChar==tok_tab||LastChar==32) { // skip blanks
        if(LastChar==9||LastChar==32)
            seen_spaces++;
        if(LastChar==10)
            seen_spaces=0;
        LastChar = _getCh();
        consume_no_ret=true;
    }
    
    
    if(LastChar=='/')
    {
        LastChar = _getCh();
        if(LastChar=='/')
        {
            LastChar = _getCh();

            while(LastChar==32||LastChar==tok_tab) 
                LastChar = _getCh();

            // if (LastChar=='$' && TryParseFnDataType())
            //     return tok_lib_dt;
            

            while(LastChar!=10 && LastChar!=tok_eof && LastChar!=tok_finish)
                LastChar = _getCh();
            if(LastChar==10)
                seen_spaces=0;
            
            return tok_commentary;
        } else
            return LastChar; 
    }



    if(LastChar=='('||LastChar==')'||LastChar=='*'||LastChar==',')
    {
        char ret_char = LastChar;
        LastChar = _getCh();
        return ret_char;
    }


    if (LastChar=='"') { // str ""
    
        LastChar = _getCh();
        running_string = "";

        while(LastChar!='"')
        {
            running_string += LastChar;
            LastChar = _getCh();
        }
        LastChar = _getCh();

        return tok_str;
    }

    
    if (isalpha(LastChar)||LastChar=='_') { // identifier
        running_string = "";

        while(isalnum(LastChar)||LastChar=='_'||LastChar==':'||LastChar=='<'||LastChar=='>')
        {
            running_string += LastChar;
            LastChar = _getCh();
        }

        if(running_string=="extern")
            return tok_extern;

        return tok_identifier;
    }

    while (LastChar==10||LastChar==tok_space||LastChar==tok_tab||LastChar==32||LastChar==13) { // skip blanks
        if(LastChar==9||LastChar==32)
            seen_spaces++;
        if(LastChar==10)
            seen_spaces=0;
        LastChar = _getCh();  
        consume_no_ret=true;
    }
    
    if (consume_no_ret)
        return LastChar;

    LastChar = _getCh();

    if(LastChar==tok_eof||LastChar==tok_finish)
        return tok_eof;

    return LastChar;
}


int LibParser::_getToken() {
    token = _getTok();
    return token;
}


void LibParser::ParseExtern() {
    is_llvm=false;
    std::string file_name = files[file_idx].string();

    _getToken(); // eat extern

    if (token!=tok_str)
      return;
    _getToken(); // eat "C"
                 

    if (running_string=="Value")
        ParseLLVMFunction();

    std::string return_type = running_string;
    _getToken(); // eat return type


    bool is_pointer = false;
    if (token=='*')
    {
      _getToken();
      is_pointer = true;
    }


    // std::cout << "\nextern \"C\" " << return_type << " " << running_string << ".\n";
    // std::cout << "" << ReverseToken(token) << ".\n";
    
    fn_name = running_string;

    _getToken(); // eat fn name



    std::vector<std::string> arg_types, arg_names;
    std::vector<int> arg_is_pointer;
    std::string last_type, last_name;
    int last_is_pointer;
    CurDefaultArgs=0;

    if(token!='(')
      return;
    // std::cout << "--------------Got parenthesis (" << ".\n";


    _getToken(); 
    bool is_var_arg=false;

    while(token!=')')
    {
        if(running_string=="..."||token=='.')
        {
            // LogBlue("Got dot at argument type on function: " + fn_name);            
            is_var_arg=true;
            while(token!=')')
                _getToken();
            break;
        }
        
        arg_types.push_back(running_string);      
        last_type = running_string;
        
        _getToken(); // eat arg type

        if (token=='*')
        {
            _getToken();
            last_is_pointer = 1;
            arg_is_pointer.push_back(1);
        } else {
            last_is_pointer = 0;
            arg_is_pointer.push_back(0);
        }
        
        arg_names.push_back(running_string);
        last_name = running_string;

        _getToken(); // eat arg name

        if(token==',')
            _getToken();
    }

    if (is_var_arg) {
        for (int i=0; i<10; ++i) {
            arg_types.push_back(last_type);
            arg_names.push_back(last_name);
            arg_is_pointer.push_back(last_is_pointer);
        }
    }
    

    token = _getToken(); // eat )

    while (token==13||token==10||token==32||token==tok_tab)
        token = _getToken();
    if (token=='{') {
        token = _getToken(); // eat {
        token = _getToken(); // Look one more token in search of return wrapping: $>
        if (token!=tok_lib_dt) {    
            while (LastChar==10||LastChar==13||LastChar==tok_tab||LastChar==32) { // skip blanks
                token = tok_space;
                LastChar = _getCh();
            }
            while (token==tok_space)
                token = _getToken();
        }
    }
    
    bool has_ret_overwrite = token==tok_lib_dt;

    if (ends_with(fn_name, "_Create")) {
        if (has_ret_overwrite)
            functions_return_data_type[fn_name] = lib_dt;
        else
            functions_return_data_type[fn_name] = Data_Tree(remove_substring(return_type, "DT_"));
        // else if (begins_with(return_type, "DT_"))
        //     functions_return_data_type[fn_name] = Data_Tree(remove_substring(return_type, "DT_"));
        // else {
        //     LogError(-1, "Create function " + fn_name + " must return a DT_ data type");
        //     std::exit(0);
        // }

    }

    LibFunction *lib_fn = new LibFunction(return_type, is_pointer, fn_name, arg_types, arg_names,
                                          arg_is_pointer, is_var_arg,
                                          has_ret_overwrite, lib_type, lib_dt, CurDefaultArgs);
    Functions[file_name].push_back(lib_fn);
    // std::cout << "\n\n";
}

LLVMFunction::LLVMFunction(std::string Name, Data_Tree ReturnType, std::vector<Data_Tree> ArgTypes, std::vector<std::string> ArgNames, int FnType) : Name(Name), ReturnType(ReturnType), ArgNames(std::move(ArgNames)), ArgTypes(std::move(ArgTypes)), FnType(FnType) {}



void LLVMFunction::HandleCreate(void *func) {

    using CreateFn = Value*(*)(Parser_Struct, Function*, std::string, std::string, Data_Tree, Value*, Value*, std::vector<std::unique_ptr<ExprAST>>&, std::vector<Value*>&);
 
    CreateFn fn = reinterpret_cast<CreateFn>(func);

    std::string dtype = remove_substring(Name, "_Create");
    struct_create_fn[dtype] = fn;
}
void LLVMFunction::HandleStandard(void *func) {
    functions_return_data_type[Name] = ReturnType;
    native_methods.push_back(Name);

    using CalleeFn = Value*(*)(Parser_Struct, Function *, std::string, Data_Tree, std::vector<Data_Tree>&, Value*, std::vector<std::unique_ptr<ExprAST>>&, std::vector<Value*>&); 
    CalleeFn fn = reinterpret_cast<CalleeFn>(func);
    llvm_callee[Name] = fn;

}

void LLVMFunction::HandleOp(void *func) {

    Name = remove_substring(Name,"OP_");

    using OpFn = Value*(*)(Parser_Struct, Function*, Data_Tree, Data_Tree, std::unique_ptr<ExprAST>&, std::unique_ptr<ExprAST>&, Value*, Value*, Value*);
    OpFn fn = reinterpret_cast<OpFn>(func);
    llvm_data_ops[Name] = fn;
}

void LLVMFunction::HandleStoreIdx(void *func) {
    using StoreIdxFn = Value*(*)(Parser_Struct, Function*, Data_Tree, Data_Tree, std::unique_ptr<ExprAST>&, std::unique_ptr<ExprAST>&, Value*, Value*, Value*, Value*);
    StoreIdxFn fn = reinterpret_cast<StoreIdxFn>(func);
    llvm_store_idx[remove_substring(Name,"_Store_Idx")] = fn;
}

void LLVMFunction::Process(void *func) {
    switch (FnType) {
        case (0):
            HandleStandard(func); 
            break;
        case (1):
            HandleOp(func);
            break;
        case (2):
            HandleCreate(func);
            Name = remove_substring(Name, "DT_");
            break;
        case (3):
            HandleStoreIdx(func); 
            break;
        default:
            break;
    }

    Function_Arg_Names[Name].push_back("0");
    Function_Arg_DataTypes[Name]["0"] = Data_Tree("Scope_Struct");

    int i=0;
    for (auto &arg_type : ArgTypes) {
        std::string arg_name = ArgNames[i++];

        Function_Arg_Names[Name].push_back(arg_name);
        Function_Arg_DataTypes[Name][arg_name] = arg_type;
    }
    Function_Required_Arg_Count[Name] = ArgTypes.size();
}

void LibParser::ParseLLVMFunction() { 
    is_llvm=true;
    std::string file_name = files[file_idx].string();

    token = _getToken(); // eat Value
    token = _getToken(); // eat *
    fn_name = running_string;

    int llvm_fn_type = 0;
    if (ends_with(file_name, "ops.cpp")||begins_with(fn_name, "OP_"))
        llvm_fn_type=1;
    else if (begins_with(fn_name, "DT_") && ends_with(fn_name, "_Create"))
        llvm_fn_type=2;
    else if (ends_with(fn_name,"_Store_Idx"))
        llvm_fn_type=3;
    else  {

    }

    CurDefaultArgs=0;


    while(token!='{')
        token = _getToken();
    // while(token!=tok_lib_dt)
    //     token = _getToken();
    token = _getToken(); 



    std::vector<std::string> arg_names;

    int arg_idx = 1;
    for (const auto &arg : LLVMFunction_Args)
        arg_names.push_back(std::to_string(arg_idx++));

    LLVMFunction *lib_fn = new LLVMFunction(fn_name, lib_dt,
                                        std::move(LLVMFunction_Args), std::move(arg_names),
                                        llvm_fn_type);
    LLVMFunctions[file_name].push_back(lib_fn);
    LLVMFunction_Args.clear();
}


void LibParser::ParseLibs() {
    token = _getToken();    

    while(file_idx<files.size())
    {  
      if (token==tok_extern)
        ParseExtern();
      token = _getToken();
    }
}

void LibParser::ImportLibs(std::string so_lib_path, std::string lib_name, bool is_default) {

    if (!has_main) { // For LSP only
        for (auto pair : Functions) {  // std::map<std::string, std::vector<LibFunction*>>
            for (auto fn : pair.second) { // std::vector<LibFunction*>>
                // std::cout << "Importing function:" << "\n";
                // fn->Print();
                fn->Add_to_Nsk_Dicts(nullptr, lib_name, is_default);
            }
        }
        return;
    }

    void *handle = dlopen(so_lib_path.c_str(), RTLD_LAZY | RTLD_GLOBAL);

    if (!handle) {
        std::string err = dlerror();
        LogError(-1, "Failed to load library " + so_lib_path + ":\n\tError:" + err);
        std::exit(0);
    }

    bool has_error=false;


    using InitFuncType = void (*)();
    for (auto pair : Functions) {  // std::map<std::string, std::vector<LibFunction*>>
        for (auto fn : pair.second) // std::vector<LibFunction*>>
        {
            dlerror(); // Clear any existing error
            void* func_ptr = dlsym(handle, fn->Name.c_str());
            const char *dlsym_error = dlerror();
            if (dlsym_error) {
                LogError(-1, "Cannot load symbol " + fn->Name + ": " + dlsym_error + ". Please, compile the .so again");
                has_error=true;
                continue;
            }
            if (!func_ptr) {
                LogError(-1, "Function " + fn->Name + " not found on library " + lib_name);
                has_error=true;
                continue;
            }
            if (begins_with(fn->Name,"initialize__"))
            {
                InitFuncType initialize_fn = reinterpret_cast<InitFuncType>(func_ptr);
                initialize_fn();
            }

            fn->Link_to_LLVM(func_ptr, handle);
            fn->Add_to_Nsk_Dicts(func_ptr, lib_name, is_default);
        }
    }
    for (auto pair : LLVMFunctions) {  // std::map<std::string, std::vector<LLVMFunction*>>
        for (auto fn : pair.second) {  // std::vector<LLVMFunction*>>
            dlerror(); // Clear any existing error
            void* func_ptr = dlsym(handle, fn->Name.c_str());
            const char *dlsym_error = dlerror();
            if (dlsym_error) {
                LogError(-1, "Cannot load symbol " + fn->Name + ": " + dlsym_error + ". Please, compile the .so again");
                has_error=true;
                continue;
            }
            fn->Process(func_ptr);
        }
    }

    if (has_error)
        std::exit(0);


    for (auto data_info : data_register_fn) {
        std::string name = data_info.first;
        if (!in_vec(name, data_tokens)) {
            data_tokens.push_back(name);
            data_name_to_type()[name] = data_type_count; 
            data_type_to_name()[data_type_count++] = name; 
        }
    }
}
