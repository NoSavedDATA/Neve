#pragma once



#include <map>
#include <string>

#include "../data_types/data_tree.h"



struct CompiledArgs {
    Data_Tree dt;
    std::string name;
    CompiledArgs(Data_Tree, std::string);
};

struct FnCompiledValuesVec {
    std::vector<int8_t> i8s;
    std::vector<int16_t> i16s;
    std::vector<int> ints;
    std::vector<int64_t> i64s;
    std::vector<float> floats;
    std::vector<std::string> strings;
};

struct FnCompiledValues {
    bool has=true;
    std::unordered_map<std::string, int8_t> i8s;
    std::unordered_map<std::string, int16_t> i16s;
    std::unordered_map<std::string, int> ints;
    std::unordered_map<std::string, int64_t> i64s;
    std::unordered_map<std::string, float> floats;
    std::unordered_map<std::string, std::string> strings;
    std::unordered_map<std::string, Data_Tree> layouts;

    bool operator<(const FnCompiledValues& rhs) const {
        return ints.size()+floats.size() < rhs.ints.size()+rhs.floats.size();
    }
};

struct Parser_Struct {
  std::string class_name="";
  std::string function_name="";
  std::string prev_function_name="";
  std::string parse_fn="";
  bool can_be_string=false;
  bool can_be_list=false, has_compiled_args=false;
  int gpu=0;
  int line=0;
  int loop_depth=0;
  int control_flow_depth=0;
  FnCompiledValues cvalues;
};

extern std::unordered_map<std::string,std::vector<CompiledArgs>> Fn_Compiled_Args;
extern std::unordered_map<std::string,std::map<FnCompiledValues,int>> Fn_Compiled_Version;
extern std::unordered_map<std::string,FnCompiledValues> Fn_Compiled_Values;
extern std::unordered_map<std::string,int> Fn_Last_Version;
