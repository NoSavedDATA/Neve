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

// extern std::string line_read;

struct TokenizerIF {
    public:
        virtual ~TokenizerIF() = default;
        int line, NumVal, LastSeenTabs=-1, SeenTabs=-1;
        std::istream* current;
        std::unique_ptr<std::istream> current_stream;
        std::string dir="", file_name, IdentifierStr;
        char cur_c=' ', LastChar=' ';

        TokenizerIF(std::string);

        virtual char get();
        virtual bool openFile(std::string);
        virtual int getToken();
};

enum TokenizerIFTokens {
    if_tok_eof = -1,
    if_tok_number = -2,
    if_tok_int = -12,
    if_tok_str = -3,
    if_tok_char = -4,
    if_tok_identifier = -11,
    if_tok_arrow = -5,
    if_tok_equal = -6,
    if_tok_diff = -7,
    if_tok_higher_eq = -8,
    if_tok_minor_eq = -9,
    if_tok_int_div = -10,
    if_tok_space = 10
};
