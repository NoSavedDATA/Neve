
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <charconv>
#include <string>


#include "../compiler_frontend/include.h"
#include "../compiler_frontend/parser_struct.h"
#include "../common/extension_functions.h"
#include "data_tree.h"


std::map<std::string, Data_Tree> functions_return_data_type;


Data_Tree::Data_Tree(std::string Type, std::vector<Data_Tree> Nested_Data) : Type(Type), Nested_Data(std::move(Nested_Data)) {}
Data_Tree::Data_Tree(std::string Type) : Type(Type) {}

bool CompareListUnkList(Data_Tree *L, Data_Tree R) {
    if((L->Type=="list"&&R.Type=="unknown_list")||L->Type=="unknown_list"&&R.Type=="list")
        return true;
    return false;
}

bool Data_Tree::IsComposite() const {
    return (Nested_Data.size()>0||Type=="layout"||is_buffer||is_array);
}
bool Data_Tree::IsBuffered() const {
    return (Type=="layout"||is_buffer||is_array);
}

int CompareArrays(const Data_Tree *L, Data_Tree R) {

    if(L->Nested_Data.size()==0||R.Nested_Data.size()==0)
        return 1;


    // return (L->Nested_Data[0].Type==R.Nested_Data[0].Type) ? 0 : 1;
    return L->Nested_Data[0].Compare(R.Nested_Data[0]);
}
int CompareVec(const Data_Tree *L, Data_Tree R) {
    if(L->Nested_Data.size()==0||R.Nested_Data.size()==0)
        return 1;
    return (L->Nested_Data[0].Compare(R.Nested_Data[0])==0) && \
           (L->Nested_Data[1].Type==R.Nested_Data[1].Type) ? 0 : 1;
}


bool CompareListRecursive(Data_Tree L, Data_Tree R) {
    
    if (L.Nested_Data.size()==0) //todo: check this condition
        return true;

    std::string list_type = L.Nested_Data[0].Type;

    if (in_str(list_type,{"list", "tuple", "dict", "array"})) {
        if (!in_str(R.Type,{"list","tuple","dict","array"}))
            return false;
        return CompareListRecursive(L.Nested_Data[0], R.Nested_Data[0]);
    }

    if(R.Type=="list")
        return list_type==R.Nested_Data[0].Type;
    
    if(R.Type=="tuple") {
        for (auto data_tree : R.Nested_Data)
        {
            if(list_type!=data_tree.Type)
                return false;
        }    
    }
    
    
    return true;
}


size_t Data_Tree::Hash() const {
    size_t hash=0;
    hash += std::hash<std::string>{}(Type);
    
    if(is_buffer)
        hash += std::hash<std::string>{}("as_buffer");
    if(is_array)
        hash += std::hash<std::string>{}("as_array");

    if (Nested_Data.size()==0)
        return hash;

    for (auto &dt : Nested_Data)
        hash += std::hash<std::string>{}(Type);

    return hash;
}

bool Data_Tree::CompareMap(Data_Tree &R) const {
    if (Nested_Data.size()==0)
        return true;
    return CheckIsEquivalent(Type, R.Nested_Data[1].Type); 
}

bool CompareListTuple(const Data_Tree *L, Data_Tree R) {

    if(L->Type!="list"||R.Type!="tuple")
        return true;

    CompareListRecursive(*L, R);
    return true;
}



bool CheckChannel(const Data_Tree *L_ptr, Data_Tree R) {
    Data_Tree L = *L_ptr;

    if(!(L.Type=="channel"||R.Type=="channel"))
        return false;

    if (L.Nested_Data.size()==0||R.Nested_Data.size()==0)
        return true;
    
    if(L.Type=="channel")
        L = L.Nested_Data[0];
    if(R.Type=="channel")
        R = R.Nested_Data[0];

    
    return L.Compare(R)==0;
}



int Data_Tree::Compare(Data_Tree other_tree) const {    
    int comparisons = 0;

    if(is_generic||other_tree.is_generic)
        return 0;

    if(Type=="void"||other_tree.Type=="void")
        return 0;

    if(Type=="Function"||other_tree.Type=="Function")
        return 0;

    if((!in_vec(Type, primary_data_tokens)||is_array) && other_tree.Type=="nullptr")
        return 0;

    if((is_array||is_buffer)!=(other_tree.is_array||other_tree.is_buffer)&&
        Type!="any"&&other_tree.Type!="any")
        return 1;


    if(Type=="vec"&&other_tree.Type=="vec")
        return CompareVec(this, other_tree);



    if (IsBuffered()&&other_tree.IsBuffered())
        return Type!=other_tree.Type;

    if(in_vec(Type, primary_data_tokens) && !IsBuffered() \
            && !other_tree.IsBuffered() && \
            in_str(other_tree.Type, primary_data_tokens) &&\
         CheckIsEquivalent(Type, other_tree.Type))
        return 0;



    

    if(Type=="any"||other_tree.Type=="any")
        return 0;

    if(Type=="charv"||other_tree.Type=="charv")
        return 0;

    if(Type=="array"&&other_tree.Type=="array")
        return CompareArrays(this, other_tree);

    if(Type=="map"&&Nested_Data.size()==0)
        return 0;
    if(Type!="map"&&other_tree.Type=="map"&&CompareMap(other_tree))
        return 0;

    if(Type=="map"&&other_tree.Type!="map"&&Nested_Data[1].Type==other_tree.Type)
        return 0;

    if(Type=="map"&&!other_tree.IsComposite())
        return Nested_Data[1].Compare(other_tree);

    if(in_vec(Type, compound_tokens)&&!in_vec(other_tree.Type, compound_tokens))
        return Nested_Data[0].Compare(other_tree);

 
    if((Nested_Data.size()==0&&other_tree.Nested_Data.size()==0) && !CheckIsEquivalent(Type, other_tree.Type))
        return comparisons+1;
     
    if(!CompareListTuple(this, other_tree))
        return comparisons+1;

    if ((Type=="list"||Type=="array")&&other_tree.Type=="tuple"||CheckChannel(this, other_tree))
        return comparisons;

    if(Nested_Data.size()!=other_tree.Nested_Data.size()){
        if ((Type=="list"&&other_tree.Type=="list")&&(Nested_Data.size()>0&&other_tree.Nested_Data.size()==0))
            return comparisons;

        // LogErrorC(-1, "Nested data has different size: " + std::to_string(Nested_Data.size()) + \
				      // "/" + std::to_string(other_tree.Nested_Data.size()) + ".\n");
        comparisons++;
    } else {
        for(int i=0; i<Nested_Data.size(); ++i)
            comparisons += Nested_Data[i].Compare(other_tree.Nested_Data[i]);
    }

    return comparisons;
}

void Data_Tree::Print(bool break_line) const {
    std::string str = toString();
    std::cout << str;
    if (is_buffer)
        std::cout << "[]";
    if (break_line)
        std::cout << "\n";
}

void print_dt_vec(std::vector<Data_Tree> dts) {
    int size = dts.size();
    if (size==0) 
        return;
    std::cout << "(";
    dts[0].Print(false);
    for (int i=1; i<size; ++i) {
        std::cout << ", ";
        dts[i].Print(false);
    }
    std::cout << ")\n";
}

std::string Data_Tree::toString() const {
    std::string str = Type; 
    if (Nested_Data.size()>0) {
        str += "<";
        Data_Tree dt = Nested_Data[0];
        str += dt.toString();
        for (int i=1; i<Nested_Data.size(); ++i) {
            dt = Nested_Data[i];
            str += ",";
            str += dt.toString();
        }
        str += ">";

    }
    return str;
}


bool Data_Tree::IsTemplate() const {
    if (Type=="layout")
        return true;
    return false;
}






std::string UnmangleVec(Data_Tree dt) {
    if (dt.Type=="channel")
        return  dt.Nested_Data[0].Type + "_channel";
    return dt.Type;
}
