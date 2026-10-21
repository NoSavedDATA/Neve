
#include "memid.h"

std::vector<int> ExprAST::GetMemId() {
    return {OM_non_obj};
}
std::vector<int> NewVecExprAST::GetMemId() {
    return Memids;
}
std::vector<int> NewExprAST::GetMemId() {
    return {MemId};
}
std::vector<int> UnaryExprAST::GetMemId() {
    return Operand->GetMemId();
}
std::vector<int> BinaryExprAST::GetMemId() {
    // return LHS->GetMemId() || RHS->GetMemId();
    if (Memids.size()>0&&Memids[0]!=OM_non_obj) {
        return Memids;
    }
    int id = RHS->GetMemId()[0];
    if (id>=0) return {id};
    id = LHS->GetMemId()[0];
    if (id>=0) return {id};
    return {OM_non_obj};
}




std::vector<int> Nameable::GetMemId() {
    // Check Depth 1 owned for borrowed

    if (Memids[0] != OM_non_obj) {
        return Memids;
    }
    if (Depth>1)
        return {OM_non_obj};

    std::string scope = parser_struct->base_name;


    if (fn_memid[scope].count(Name)==0)
        return {OM_non_obj};
    // std::cout << "for " << Name << "\n";
    return {fn_memid[scope][Name]};
}


std::vector<int> NameableCall::GetMemId() {
    // std::cout << "as call " << MemId << "\n";
    return {MemId};
}



bool Nameable::GetIsView() {
    bool is_view=false;
    if (in_vec(GetMemId()[0], fn_views[parser_struct->function_name])) {
        is_view=true;
    }
    std::string scope = (Depth==1) ? parser_struct->function_name
                                   : Inner->GetDataTree().Type;
    if (in_vec(Name, fn_class_views[scope])) {
        is_view=true;
    }
    if (Depth>1)
        return is_view||Inner->GetIsView();
    return is_view;
}

bool NameableIdx::GetIsView() {
    return Inner->GetIsView();
}
bool NameableCall::GetIsView() {
    return Inner->GetIsView();
}

