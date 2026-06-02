#include <cstddef>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../data_types/data_tree.h"

bool Shall_Exit = false;


// Tensor related
std::vector<std::string> return_tensor_functions, return_tensor_methods, return_tensor_fn, native_modules,
return_pinned_methods, vararg_methods, string_methods, native_methods, native_functions, native_fn,
return_string_fn, threaded_tensor_functions, require_scope_functions, notators_str, template_fn;

std::map<std::string, std::string> reverse_ops;

std::vector<std::string> user_cpp_functions;
std::map<std::string, int> Function_Arg_Count;
std::map<std::string, int> Function_Required_Arg_Count;
std::map<std::string, std::map<std::string, Data_Tree>> Function_Arg_DataTypes;
std::map<std::string, std::map<std::string, std::string>> Function_Arg_Types;
std::map<std::string, std::vector<std::string>> Function_Arg_Names;
std::vector<std::string> Sys_Arguments;


std::map<std::string, std::string> elements_type_return, ops_type_return;
std::map<int, std::string> op_map;
std::vector<std::string> op_map_names;
std::string CurrentFile = "main";

bool has_main=false;
bool IsJIT = true;
int Array_Size=-1;

std::map<char, int> BinopPrecedence;



std::unordered_map<std::string, int> Classes;
std::map<size_t, std::vector<char *>> CharPool;


std::map<std::string, std::vector<std::string>> Equivalent_Types = {{"int", {"float", "i64"}},
                                                                    {"i64", {"int"}},
                                                                    {"char", {"i8"}}};

std::vector<std::string> int_types = {"int", "i64", "i8", "i16", "char"};


std::unordered_map<std::string, uint16_t> data_name_to_size = {{"int", 4}, {"i64", 8}, {"i8", 1}, {"char", 1}, {"i16", 2}, {"float", 4}, {"bool", 1}, {"any", 8}, {"double", 8}, {"str", 16}, {"str_view", 16}, {"Function", 8}, {"array", 8}};
std::unordered_map<uint16_t, uint16_t> data_type_to_size; 
 

std::unordered_map<std::string, uint16_t>& data_name_to_type() {
    static auto* map = new std::unordered_map<std::string, uint16_t>();
    return *map;
}
std::unordered_map<uint16_t, std::string>& data_type_to_name() {
    static auto* map = new std::unordered_map<uint16_t, std::string>();
    return *map;
}

// std::unordered_map<std::string, uint16_t> data_name_to_type = {{"int", 2}, {"float", 3}, {"bool", 4}, {"double", 5},
//                                                                {"i8", 6}, {"i16", 7}, {"i64", 8},
//                                                                {"str", 100},
//                                                                {"list", 101},
//                                                                {"tuple", 107}, {"map", 108}, {"channel", 109},
//                                                                {"array", 102}, {"map_node", 103},
//                                                                {"char", 104},  {"charv", 105},
//                                                                {"vec", 106},
//                                                                {"str_view", 110}, {"any", 111}, {"Function", 112}};
// std::unordered_map<uint16_t, std::string> data_type_to_name;

uint16_t data_type_count=113;



std::vector<std::string> data_tokens = {"int", "bool", "str", "str_vec", "float_vec",
                                        "tuple", "any", "float_ptr",
										"list", "map", "array", 
                                        "float", "int_vec", "char", "charv", "vec", "i16", "i64", "i8", "str_view"};
std::vector<std::string> compound_tokens = {"tuple", "channel", "list", "array", "map", "vec", "Function"};
std::vector<std::string> primary_data_tokens = {"vec", "int", "float", "bool", "foreach_control_var", "i64", "i8", "i16", "char"};


std::vector<uint16_t> primary_data_types = {2, 3, 4, 5, 6, 15, 16, 17, 18, 21};
std::vector<uint16_t> compound_types = {6, 7, 8, 9, 12};
