#pragma once

#include <cstdint>
#include <limits>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>

#include "../data_types/data_tree.h"


extern std::vector<std::string> return_tensor_functions, return_tensor_methods, return_tensor_fn, native_modules,
return_pinned_methods, vararg_methods, string_methods, native_methods, native_functions, native_fn, tensor_inits,
return_string_fn, threaded_tensor_functions, int_types, require_scope_functions, notators_str, user_cpp_functions, template_fn;

extern std::vector<int> functional_tokens;
extern std::vector<std::string> prebuild_functions;
extern std::map<std::string, int> Function_Arg_Count;
extern std::map<std::string, int> Function_Required_Arg_Count;
extern std::map<std::string, std::map<std::string, std::string>> Function_Arg_Types;
extern std::map<std::string, std::map<std::string, Data_Tree>> Function_Arg_DataTypes;
extern std::map<std::string, std::vector<std::string>> Function_Arg_Names;

extern std::map<std::string, std::string> elements_type_return, ops_type_return;
extern std::map<int, std::string> op_map;
extern std::vector<std::string> op_map_names;
extern std::vector<std::string> int_fn_values;

constexpr int32_t TERMINATE_VARARG = -2147483647;
constexpr int32_t COPY_TO_END_INST = 0x7FADBEEF;

constexpr int32_t MAX_THREADS = 48;


extern bool Shall_Exit;
extern int Array_Size;

extern bool has_main;
extern bool IsJIT;

extern std::string CurrentFile;

extern std::map<char, int> BinopPrecedence;

extern std::unordered_map<std::string, int> gpu_fn, gpu_ffi, kernel_fn;

std::unordered_map<uint16_t, std::string>& data_type_to_name();
std::unordered_map<std::string, uint16_t>& data_name_to_type();

// extern std::unordered_map<uint16_t, std::string> data_type_to_name;
// extern std::unordered_map<std::string, uint16_t> data_name_to_type;

extern std::unordered_map<std::string, uint16_t> data_name_to_size;
extern std::unordered_map<uint16_t, uint16_t> data_type_to_size;
extern uint16_t data_type_count;

extern std::map<std::string, std::string> functions_return_type, reverse_ops;

extern std::unordered_map<std::string, int> ClassSize;
extern std::unordered_map<std::string, int> Classes;

extern std::vector<std::string> Global_Uniques;

extern std::map<std::string, std::vector<std::string>> Equivalent_Types;

extern std::vector<std::string> data_tokens, constants, compound_tokens, primary_data_tokens;
extern std::vector<uint16_t> primary_data_types;
extern std::vector<uint16_t> compound_types;
