
#include <ctype.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <stack>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "../runtime/common/extension_functions.h"
#include "../runtime/compiler_frontend/logging_v.h"
#include "tokenizer.h"


namespace fs = std::filesystem;

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//


// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known words.


std::map<int, std::string> token_to_string = {
  { tok_eof, "eof" },
  { tok_finish, "finish" },

  // functions/classes
  { tok_def, "def" },
  { tok_constructor, "ctor" },
  { tok_class, "class" },
  { tok_import, "import" },
  { tok_proto, "proto" },
  { tok_op, "op" },
  { tok_self, "self" },
  { tok_class_attr, "object attr" },
  { tok_extern, "extern" },

  // primary
  { tok_identifier, "tok identifier" },
  { tok_number, "tok number" },
  { tok_hexa, "tok hexa (0x)" },
  { tok_str, "tok str `` ''" },
  { tok_char, "tok char `` ''" },
  { tok_var, "var" },
  { tok_int, "int number" },
  { tok_lutlo, "lut_lo" },
  { tok_luthi, "lut_hi" },

  { tok_new, "tok new" },

  // control
  { tok_if, "if" },
  { tok_then, "then" },
  { tok_else, "else" },
  { tok_for, "for" },
  { tok_while, "while" },
  { tok_break, "break" },
  { tok_spawn, "spawn" },
  { tok_channel, "channel" },
  { tok_async, "async" },
  { tok_asyncs, "asyncs" },
  { tok_async_finish, "finish finish/async" },
  { tok_tab, "tok tab" },
  { tok_return, "tok return"},
  { tok_tuple, "tok tuple"},
  { tok_list, "tok list"},
  { tok_array, "tok array"},
  { tok_map, "tok map"},
  { tok_as, "tok as"},
  { tok_in, "tok in"},

  // operators
  { tok_binary, "tok binary" },
  { tok_unary, "tok unary" },

  { tok_main , "tok main" },


  { tok_post_class_attr_attr, ".attr."},
  { tok_post_class_attr_identifier, ".identifier"},
  
  // var definition
  { tok_attr_var, "tok attr var"},


  // specials
  
  { tok_commentary, "// commentary" },
  { tok_lib_dt, "$>" },

  { tok_global, "global"},
  { tok_no_grad, "no_grad"},

  { tok_arrow, "<-"},
  

  { 10, "tok space"},
  { tok_space, "tok space"},
  { 32, "blank space"},
  { 13, "carriage return"},


  { tok_and, "tok and" },
  { tok_not, "tok not" },
  { tok_or, "tok or" },
  { tok_xor, "tok xor" },
  
  { '.', "dot<.>" },

  { 36, "$" },

  { 40, "(" },
  { 41, ")" },

  { 42, "*" },
  { 43, "+" },
  { 44, "," },
  { 45, "-" },
  { 47, "/" },

  { 48, "0" },
  { 49, "1" },
  { 50, "2" },
  { 51, "3" },
  { 52, "4" },
  { 53, "5" },
  { 54, "6" },
  { 55, "7" },
  { 56, "8" },
  { 57, "9" },
  { 58, ":" },
  { 59, ";" },
  { 60, "<" },
  { 61, "=" },
  { 62, ">" },
  { 64, "@" },


  { 91, "[" },
  { 93, "]" },

  { 123, "{" },
  { 125, "}" },

  { tok_lshift, "<<" },
  { tok_rshift, ">>" },
  { tok_equal, "==" },
  { tok_diff, "!=" },
  { tok_int_div, "//" },
  { tok_higher_eq, ">=" },
  { tok_minor_eq, "<=" },
  { tok_mod, "//" },


  { static_cast<int>(','), "," },
  { static_cast<int>(';'), ";" },
  { static_cast<int>(':'), ":" },

  { static_cast<int>('a'), "a" },
  { static_cast<int>('b'), "b" },
  { static_cast<int>('c'), "c" },
  { static_cast<int>('d'), "d" },
  { static_cast<int>('e'), "e" },
  { static_cast<int>('f'), "f" },
  { static_cast<int>('g'), "g" },
  { static_cast<int>('h'), "h" },
  { static_cast<int>('i'), "i" },
  { static_cast<int>('j'), "j" },
  { static_cast<int>('k'), "k" },
  { static_cast<int>('l'), "l" },
  { static_cast<int>('m'), "m" },
  { static_cast<int>('n'), "n" },
  { static_cast<int>('o'), "o" },
  { static_cast<int>('p'), "p" },
  { static_cast<int>('q'), "q" },
  { static_cast<int>('r'), "r" },
  { static_cast<int>('s'), "s" },
  { static_cast<int>('t'), "t" },
  { static_cast<int>('u'), "u" },
  { static_cast<int>('v'), "v" },
  { static_cast<int>('w'), "w" },
  { static_cast<int>('x'), "x" },
  { static_cast<int>('y'), "y" },
  { static_cast<int>('z'), "z" },

};
std::vector<char> ops = {'+', '-', '*', '/', '@', '=', '>', '<', 10, -14, ',', '(', ')', ';', tok_equal,
                         tok_diff, tok_higher_eq, tok_minor_eq, tok_offby};
std::vector<char> terminal_tokens = {';', tok_constructor, tok_def, tok_extern, tok_class, tok_eof};

extern std::vector<std::string> LLVM_IR_Functions = {"pow", "sqrt"};

std::map<std::string, char> string_tokens = {{"var", tok_var}, {"self", tok_self}, {"def", tok_def},
                                             {"ctor", tok_constructor}, {"class", tok_class}, {"extern", tok_extern},
                                             {"import", tok_import}, {"if", tok_if}, {"then", tok_then},
                                             {"else", tok_else}, {"for", tok_for},
										     {"while", tok_while}, {"async", tok_async}, {"asyncs", tok_asyncs},
                                             {"finish", tok_async_finish},
											 {"in", tok_in}, {"global", tok_global}, {"no_grad", tok_no_grad},
                                             {"lock", tok_lock},
											 {"unlock", tok_unlock}, {"binary", tok_binary}, {"unary", tok_unary},
                                             {"return", tok_ret},
											 {"as", tok_as}, {"spawn", tok_spawn}, {"channel", tok_channel},
                                             {"main", tok_main},
                                             {"nil", tok_nil},
                                             {"lut_lo", tok_lutlo}, {"lut_hi", tok_luthi},
                                             {"any", tok_any},
                                             {"proto", tok_proto}, {"operation", tok_op},
                                             {"and", tok_and},
										     {"not", tok_not}, {"or", tok_or}, {"xor", tok_xor},
                                             {"break", tok_break},
                                             {"continue", tok_continue},
                                             {"offby", tok_offby},
                                             {"new", tok_new}};

std::string IdentifierStr; // Filled in if tok_identifier
float NumVal;             // Filled in if tok_number
int HexaVal;
bool BoolVal;

std::string ReverseToken(int _char) {
  if (_char==tok_identifier||_char==tok_data||_char==tok_struct)
    return IdentifierStr;

  return token_to_string[_char];
}

int LineCounter = 1;

int SeenTabs = 0;
int LastSeenTabs = 0;


Tokenizer::Tokenizer(std::string file) : TokenizerIF(file) {
}

Tokenizer::Tokenizer(std::string file, std::unique_ptr<Tokenizer> inner) 
        : TokenizerIF(file), inner(std::move(inner)) {
}


bool import_NEVE_File(std::string filename) {
    // std::cout << "current " << tokenizer->file_name << "\n";
    // std::cout << "import of " << filename << "\n\n";
    
    tokenizer->LastToken = CurTok;
    tokenizer = std::make_unique<Tokenizer>(filename, std::move(tokenizer));
    return true;
}



// Tokenizer tokenizer = Tokenizer();
std::unique_ptr<Tokenizer> tokenizer;




/// get_token - Return the next token from standard input.
static int get_token(bool block) {
  static int LastChar = ' ';
  if (block)
    return tok_space;


  // Skip any whitespace and backspace.  
  while (LastChar==32 || LastChar==tok_tab || LastChar==13) {
    LastChar = tokenizer->get();
  }
  
    

  if (LastChar=='[')
  {
    LastChar = tokenizer->get();
    return '[';
  }

  if (LastChar=='\'') {
    LastChar = tokenizer->get();

    if (LastChar == '\\') {              // escape sequence
        LastChar = tokenizer->get();
        switch (LastChar) {
            case 'n': NumVal = '\n'; break;
            case 't': NumVal = '\t'; break;
            case '0': NumVal = '\0'; break;
            case 'r': NumVal = '\r'; break;
            case '\\': NumVal = '\\'; break;
            case '\'': NumVal = '\''; break;
            default:
                LogErrorC(tokenizer->Line, "Unknown escape sequence");
        }
    } else {
        NumVal = LastChar;               // normal char
    }

    LastChar = tokenizer->get();

    if (LastChar!='\'')
        LogErrorC(tokenizer->Line, "Could not find matching '");

    LastChar = tokenizer->get();
    return tok_char;
}

  if (LastChar=='"')
  {

    LastChar = tokenizer->get();
    if (LastChar=='"') {
      IdentifierStr = "";
      LastChar = tokenizer->get();
      return tok_str;
    }
    IdentifierStr = LastChar;

    while (true)
    {
      LastChar = tokenizer->get();
      if(LastChar=='"')
        break;
      IdentifierStr += LastChar;
    }
    LastChar = tokenizer->get();
    
    return tok_str;
  }


  if(LastChar=='.') 
  {
    LastChar = tokenizer->get();
    return '.';
  }
  

  if (isalpha(LastChar) || LastChar=='_') { // identifier: [a-zA-Z][a-zA-Z0-9]*
    // std::cout << "got alpha " << LastChar<< ".\n";
    IdentifierStr = LastChar;
    bool name_ok=true;
    while(true) {
      LastChar = tokenizer->get();
      if (LastChar=='['||LastChar=='.')
        break;
      
      if(isalnum(LastChar) || LastChar=='_') {
        IdentifierStr += LastChar;
        continue;
      }        
      break;
    }

 
    if (in_vec(IdentifierStr, compound_tokens))
      return tok_struct;
    if (in_vec(IdentifierStr, data_tokens))
      return tok_data;
    if(string_tokens.count(IdentifierStr)>0)
      return string_tokens[IdentifierStr];
    if (IdentifierStr=="true"||IdentifierStr=="false") {
      if(IdentifierStr=="true")
        BoolVal = true;
      if(IdentifierStr=="false")
        BoolVal = false;
      return tok_bool;
    }
    if (IdentifierStr == "glob")
      IdentifierStr = "_glob_b_";
    if (IdentifierStr == "sleep")
      IdentifierStr = "__slee_p_";
    if (IdentifierStr == "tanh")
      IdentifierStr = "_tanh";
    return tok_identifier;
  }
  // if (LastChar=='@') {
  //   LastChar = tokenizer->get();
  //   std::string NumStr;
  //   do {
  //     NumStr += LastChar;
  //     LastChar = tokenizer.get();
  //   } while(isdigit(LastChar));
  //   NumVal = strtod(NumStr.c_str(), nullptr);
  //   return tok_int;
  // }
  
  if (isdigit(LastChar)) { // Number: [-.]+[0-9.]+
    bool is_float=false;
    bool is_hexa=false;
    
    std::string NumStr;
    if (LastChar == '-') { // Check for optional minus sign
      NumStr += LastChar;
      LastChar = tokenizer->get();
    }
    do {
      if(LastChar=='.')
        is_float=true;
      if(LastChar=='x')
        is_hexa=true;

      NumStr += LastChar;
      LastChar = tokenizer->get();
    } while (isdigit(LastChar) || LastChar == '.' || LastChar=='x' || (is_hexa&&isalpha(LastChar)));

    if (is_hexa)
        HexaVal = strtoull(NumStr.c_str(), nullptr, 16);
    else
        NumVal = strtod(NumStr.c_str(), nullptr);
    
    if (is_float) return tok_number;
    if (is_hexa) return tok_hexa;
    return tok_int;
  }

  if (LastChar == '#') {
    // Comment until end of line.
    tokenizer->Line++;
    do {
      LastChar = tokenizer->get();
    }
    while (LastChar != EOF && LastChar != '\n' && LastChar != 10 && LastChar != '\r');

    if (LastChar != EOF)
      return get_token(false);
  }

  // Check for end of file.  Don't eat the EOF.
  if (LastChar == EOF) {
    LastChar = ' ';
    return tok_eof;
  }
  

  // Otherwise, just return the character as its ascii value.
  int ThisChar = LastChar;


  if (ThisChar==10&&!tokenizer->can_see_space) {
      LastChar = tokenizer->get();
      return tok_space;
  }
  
  if (ThisChar==10 || LastChar==tok_tab) {
    int seen_spaces=0;

    while(LastChar==10 || LastChar==tok_tab || LastChar==32) {
      if(LastChar==10)
        tokenizer->Line++;
      if(ThisChar==10) {
        LastSeenTabs = SeenTabs;
        SeenTabs = 0;
        seen_spaces = 0;
      }
      if (LastChar==tok_tab)
        SeenTabs+=1;
      if (LastChar==32)
        seen_spaces++;
      if (seen_spaces==3) {
        seen_spaces=0;
        SeenTabs+=1;
      }

      ThisChar = (int)LastChar;
      LastChar = tokenizer->get(); 
    }
    if (ThisChar==10&&isalpha(LastChar))
        SeenTabs = 0;

    return tok_space;
  }


  LastChar = tokenizer->get();
  int otherChar = LastChar;


  if(ThisChar=='<' && LastChar=='-') {
    LastChar = tokenizer->get();
    return tok_arrow;
  }

  if(ThisChar=='<' && LastChar=='|') {
    LastChar = tokenizer->get();
    return tok_lshift;
  }
  if(ThisChar=='|' && LastChar=='>') {
    LastChar = tokenizer->get();
    return tok_rshift;
  }

  if (ThisChar=='=' && otherChar=='=') {
    LastChar = tokenizer->get();
    return tok_equal;
  }
  if (ThisChar=='!' && otherChar=='=') {
    LastChar = tokenizer->get();
    return tok_diff;
  }
  if (ThisChar=='>' && otherChar=='=') {
    LastChar = tokenizer->get();
    return tok_higher_eq;
  }
  if (ThisChar=='<' && otherChar=='=') {
    LastChar = tokenizer->get();
    return tok_minor_eq;
  }

  if((ThisChar=='/')&&(otherChar == '/')) {
    LastChar = tokenizer->get();
    return tok_int_div;
  }

  //std::cout << "Post char: " << ReverseToken(ThisChar) << "\n";

  // else: return ascii number of the character.
  return ThisChar;
}



/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
int CurTok;
int getNextToken(bool block) {
  CurTok = get_token(block); 
  return CurTok;
}




void get_tok_until_space() {
  if(CurTok!=tok_space&&tokenizer->cur_c!=10) // gets until the \n before switching files
  {
    // std::cout << "CurTok: " << ReverseToken(CurTok) << " / " << CurTok  << " / " << std::to_string(int(tokenizer->cur_c)) << ".\n";
    char c=' ';
    while(c!=10)
    {
      int _c = c;
      c = tokenizer->get();

      // std::cout << "Get " << c << ".\n";
    }
  }
  CurTok = tok_space;
  // LineCounter++;
}




/// get_tokenPrecedence - Get the precedence of the pending binary operator token.
int get_tokenPrecedence() {
  if (CurTok==tok_space)
  {
    // if (CurTok==10)
    //   LineCounter++;
    return 1;
  }


  if (BinopPrecedence.find(CurTok) == BinopPrecedence.end()) // if not found
    return -1;

  // Make sure it's a declared binop.
  int TokPrec = BinopPrecedence[CurTok];
  if (TokPrec <= 0)
    return -1;
  return TokPrec;
}
