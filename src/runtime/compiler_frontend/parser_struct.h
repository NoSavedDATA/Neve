#pragma once



#include <map>
#include <string>

#include "../data_types/data_tree.h"
#include "global_vars.h"




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

    bool operator==(const FnCompiledValues& rhs) const {
        if (rhs.ints.size()!=ints.size())
            return false;
        if (rhs.layouts.size()!=layouts.size())
            return false;

        for (auto &p : ints) {
            if (rhs.ints.count(p.first)==0)
                return false;
            auto it = rhs.ints.find(p.first);
            int rhs_val = it->second;
            if (p.second!=rhs_val)
                return false;
        }

        for (auto &p : layouts) {
            if (rhs.layouts.count(p.first)==0)
                return false;
            Data_Tree dt=p.first;
            auto it = rhs.layouts.find(p.first);
            const Data_Tree& other_layout = it->second;
            if (dt.Nested_Data.size()!=other_layout.Nested_Data.size())
                return false;
            for (int i=0; i<dt.Nested_Data.size(); ++i) {
                if (dt.Nested_Data[i].Type!=other_layout.Nested_Data[i].Type)
                    return false;
            }
        }
        
        return true;
    }
};


struct CompValHasher {
    std::size_t operator()(const FnCompiledValues& v) const {
        size_t hash = 0;

        for (auto &p : v.ints)
            hash += p.second;
        
        for (auto &p : v.layouts)
            hash += data_name_to_type()[p.second.Nested_Data[0].Type];

        return hash;
    }
};

struct CompValEqual {
    bool operator()(const FnCompiledValues& a,
                    const FnCompiledValues& b) const {
        return a == b;
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
  Parser_Struct *Copy();
};

extern std::unordered_map<std::string,std::unordered_map<FnCompiledValues,int,CompValHasher,CompValEqual>> Fn_Compiled_Version;
extern std::unordered_map<std::string,FnCompiledValues> Fn_Compiled_Values;
extern std::unordered_map<std::string,int> Fn_Last_Version;
