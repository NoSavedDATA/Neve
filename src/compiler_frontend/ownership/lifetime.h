#pragma once

#include "include.h"
#include <cstdint>




ExprBranch FormatLifetime(ExprBranch branch, int memid); 
ExprBranch GetLifetime(Parser_Struct *parser_struct,
        ExprAST *borrowed, std::unordered_map<int,ExprBranch> &memid_to_branch); 
ExprBranch GetLifetime(Parser_Struct *parser_struct,
        ExprAST *owner_expr, ExprAST *borrowed,
        std::unordered_map<int,ExprBranch> &memid_to_branch,
        bool verbose=false); 



int innermost_cstmt(std::string fn_name,
                    int cstmt); 

bool match_cstmt_has_loop(std::string fn_name,
                    int tgt_cstmt, int cstmt); 

bool match_cstmt_parent(std::string fn_name,
                    int tgt_cstmt, int cstmt); 


