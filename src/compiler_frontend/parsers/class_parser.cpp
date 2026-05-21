#include <ctype.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <stack>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "../../runtime/common/extension_functions.h"
#include "../../runtime/compiler_frontend/logging_v.h"
#include "../include.h"




namespace fs = std::filesystem;

static void ParseImport(std::unique_ptr<TokenizerClass> &class_tokenizer) {
    int tok = class_tokenizer->getToken();

    std::string lib_name = class_tokenizer->IdentifierStr;
    tok = class_tokenizer->getToken();
    while (tok=='.') {
        tok = class_tokenizer->getToken();
        lib_name += "/" + class_tokenizer->IdentifierStr;
        tok = class_tokenizer->getToken();
    }

    std::string full_path_lib = class_tokenizer->dir+"/"+lib_name+".nv";

    std::string lib_path = std::getenv("NEVE_LIBS");
    std::string as_include = lib_path + "/" + lib_name + "/include.nv";


    if (fs::exists(full_path_lib)) {
        ParseClasses(full_path_lib);
    } else if (fs::exists(as_include)) {
        ParseClasses(as_include);
    } else {
        LogErrorC(-1, "Could not find include.nv for " + lib_name);
    }
}

static Data_Tree ParseDataTree(std::unique_ptr<TokenizerClass> &class_tokenizer, int &tok) {
    std::string type = class_tokenizer->IdentifierStr;
    Data_Tree ty = Data_Tree(type);
    tok = class_tokenizer->getToken();

    if (tok=='[') {
        tok = class_tokenizer->getToken();
        int num = class_tokenizer->NumVal;
        ty.Nested_Data.push_back(Data_Tree(std::to_string(num)));
        while (tok==',') {
            tok = class_tokenizer->getToken();
            ty.Nested_Data.push_back(Data_Tree(std::to_string(class_tokenizer->NumVal)));
        }
        tok = class_tokenizer->getToken();
    }

    if (tok=='<') {
        tok = class_tokenizer->getToken();
        ty.Nested_Data.push_back(ParseDataTree(class_tokenizer, tok));

        while (tok==',') {
            tok = class_tokenizer->getToken();
            ty.Nested_Data.push_back(ParseDataTree(class_tokenizer, tok));
        }
        class_tokenizer->getToken(); // eat >
    }
    // std::cout << "\n\ntype:" << "\n";
    // ty.Print();

    return ty;
}

static void ParseDef(std::unique_ptr<TokenizerClass> &class_tokenizer, std::string class_name) {
    if (class_tokenizer->SeenTabs==0)
        return;

    int tok = class_tokenizer->getToken(); // eat def
    Data_Tree type;
    type = ParseDataTree(class_tokenizer, tok); // eat dt
    
    std::string fn_name = class_name + "_" + class_tokenizer->IdentifierStr;
    functions_return_data_type[fn_name] = type;

    tok = class_tokenizer->getToken(); // eat name

    // std::cout << "fn " << fn_name << "\n";
    // std::cout << "ret" << "\n";
    // type.Print();
    // std::cout << "\n";
    

    Function_Arg_Names[fn_name].push_back("scope_struct");
    Function_Arg_DataTypes[fn_name]["scope_struct"] = Data_Tree("Scope_Struct");

    tok = class_tokenizer->getToken(); // eat (
    while (true) {
        if (tok==')')
            break;

        std::string type = class_tokenizer->IdentifierStr;

        Data_Tree ty;
        if (in_vec(type, {"t", "s", "i", "f", "b"})) {
            if (type=="t")
                ty = Data_Tree("tensor");
            else if (type=="i")
                ty = Data_Tree("int");
            else if (type=="b")
                ty = Data_Tree("bool");
            else if (type=="f")
                ty = Data_Tree("float");
            else if (type=="s")
                ty = Data_Tree("str");
            tok = class_tokenizer->getToken(); 
        }
        else
            ty = ParseDataTree(class_tokenizer, tok);


        std::string var_name = class_tokenizer->IdentifierStr;
        tok = class_tokenizer->getToken(); 

        
        Function_Arg_Names[fn_name].push_back(var_name);
        Function_Arg_DataTypes[fn_name][var_name] = ty;

        
        if (tok==',')
            tok = class_tokenizer->getToken(); 

    }

}


static std::string ParseClass(std::unique_ptr<TokenizerClass> &class_tokenizer) {
    int tok = class_tokenizer->getToken();
    std::string _class = class_tokenizer->IdentifierStr;
    Classes[_class] = 1;
    // std::cout << "\n\n--" << _class << "\n";
    
    tok = class_tokenizer->getToken();
    while (tok==10)
        tok = class_tokenizer->getToken();

    while(tok==class_tok_identifier) {
        Data_Tree type;
        type = ParseDataTree(class_tokenizer, tok);
            
        std::string var_name = class_tokenizer->IdentifierStr;
        data_typeVars[_class][var_name] = type;

        // std::cout << " " << var_name << ",";
        tok = class_tokenizer->getToken();
        while (tok==',') {
            tok = class_tokenizer->getToken();
            var_name = class_tokenizer->IdentifierStr;
            // std::cout << " " << var_name << ",";
            data_typeVars[_class][var_name] = type;
            tok = class_tokenizer->getToken();
        }

        while (tok==10)
            tok = class_tokenizer->getToken();
    }
    
    return _class;
}


void ParseClasses(std::string fname) {
    std::unique_ptr<TokenizerClass> class_tokenizer;
    class_tokenizer = std::make_unique<TokenizerClass>(fname);
    
    std::string last_class="";
    int tok = 0;
    while (tok!=class_tok_eof) {
        tok = class_tokenizer->getToken();
        switch (tok) {
            case(class_tok_import):
                ParseImport(class_tokenizer);
                break;
            case(class_tok_class):
                last_class = ParseClass(class_tokenizer);
                break;
            case(class_tok_def):
                ParseDef(class_tokenizer, last_class);
                // last_class = ParseClass(class_tokenizer);
                break;
            default:
                break;
        }
    }
}

