#pragma once



#include <map>
#include <memory>
#include <string>

#include "../data_types/data_tree.h"
#include "global_vars.h"





struct FnCompiledValues {
    bool has=true;
    std::unordered_map<std::string, int8_t> i8s;
    std::unordered_map<std::string, int16_t> i16s;
    std::unordered_map<std::string, int> ints;
    std::unordered_map<std::string, int64_t> i64s;
    std::unordered_map<std::string, float> floats;
    std::unordered_map<std::string, std::string> strings;
    std::unordered_map<std::string, Data_Tree> layouts;
    std::unordered_map<std::string, Data_Tree> dts;

    void AddInt(std::string key, int val) {
        ints[key] = val;
        dts[key] = Data_Tree("int");
    }

    bool operator==(const FnCompiledValues& rhs) const {
        // todo: add other cvalues
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
        // todo: add other cvalues
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
  bool can_be_list=false, has_compiled_args=false, has_owned_pool=false;
  bool borrow_ret=false;
  std::shared_ptr<int> owned_id = std::make_shared<int>(0);
  std::shared_ptr<int> mem_id = std::make_shared<int>(0);
  std::shared_ptr<uint64_t> control_stmt_id = std::make_shared<uint64_t>(2);
  int owned_id_arg_offset = 0, memid_arg_offset=0;
  int gpu=0;
  int line=0;
  int scope_depth=0;
  int control_flow_depth=0, prev_branch_id=-1;
  uint64_t branch_id=2;
  FnCompiledValues cvalues;
  std::vector<std::tuple<std::string, std::string, Data_Tree>> dyn_args;
  std::unordered_map<std::string, int> dyn_args_dict;

  Parser_Struct *Copy();
  bool has_own() {
    return *owned_id>0;
  }

  std::vector<std::string> get_arg_names() {
    std::vector<std::string> v = fn_argnames[function_name];
    if (v.size()==0)
        return v;
    if (v[0]=="scope_struct") {
        std::vector<std::string> ret(v.begin()+1, v.end());
        v = ret;
    }
    return v;
  }
};

extern std::unordered_map<std::string,std::unordered_map<FnCompiledValues,int,CompValHasher,CompValEqual>> Fn_Compiled_Version;
extern std::unordered_map<std::string,FnCompiledValues> Fn_Compiled_Values;
extern std::unordered_map<std::string,int> Fn_Last_Version;
