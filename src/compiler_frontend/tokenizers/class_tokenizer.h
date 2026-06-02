#pragma once


#include <iostream>
#include <filesystem>
#include <fstream>
#include <stack>
#include <string>
#include <map>
#include <memory>
#include <vector>
#include <unordered_map>

#include "tokenizer_if.h"

struct TokenizerClass : TokenizerIF {
    
    public:
        TokenizerClass(std::string);
        int getToken() override;
};

enum TokenizerClassTokens {
    class_tok_eof = -1,
    class_tok_class = -2,
    class_tok_commentary = -3,
    class_tok_space = 10,
    class_tok_identifier = -4,
    class_tok_import = -5,
    class_tok_type = -6,
    class_tok_int = -7,
    class_tok_float = -8,
    class_tok_def = -9,
    class_tok_ctor = -10,
    class_tok_string = -11,
};
