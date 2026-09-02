#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>



struct Data_Tree {
    std::vector<Data_Tree> Nested_Data;
    std::string Type="";
    bool empty=true, is_array=false, is_buffer=false, retry=false,
         is_generic=false, is_smem=false;
    
    Data_Tree() = default;
    Data_Tree(std::string, std::vector<Data_Tree> nested_data);
    Data_Tree(std::string);


    bool CompareMap(Data_Tree&) const;
    bool IsTemplate() const;
    int Compare(Data_Tree) const;
    size_t Hash() const;
    void Print(bool break_line=true) const;
    std::string toString() const;
    bool IsComposite() const;
    bool IsPrimary() const;
    bool IsInteger() const;
    bool IsBuffered() const;
    bool HasGeneric() const;
    bool Has(std::string type) const;
};




extern std::map<std::string, Data_Tree> functions_return_data_type;


Data_Tree GenericUnmangleType(Data_Tree dt, Data_Tree generic_dt);
std::string UnmangleVec(Data_Tree dt);
void print_dt_vec(std::vector<Data_Tree> dt);
