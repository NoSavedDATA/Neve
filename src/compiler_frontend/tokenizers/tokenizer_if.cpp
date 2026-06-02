
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

#include "tokenizer_if.h"




namespace fs = std::filesystem;





//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//



// TokenizerIF::TokenizerIF(std::string file) : current(&std::cin) {
TokenizerIF::TokenizerIF(std::string file) {
    openFile(file);
}

// std::string line_read;

char TokenizerIF::get() {
    while (true) {
        if (!current) return if_tok_eof;
 
        char c = current->get();

        // line_read += c;
        if (c != EOF)
          return c;

        current = nullptr;
        return if_tok_eof;
    }
}


bool TokenizerIF::openFile(std::string filename) {
    auto file = std::make_unique<std::ifstream>(filename);
    if (!file->is_open())
    {
      std::cout << "Failed to open file: " << filename << "\n";
      return false;
    }
    file_name = filename;
    std::string base = fs::path(filename).parent_path().string();
    if(filename[0]=='/')
      dir = base;
    else if (base=="")
      dir = "";
    else
      dir = dir + "/" + base;
    current = file.get();
    current_stream = std::move(file);

    return true;
}





/// get_token - Return the next token from standard input.
int TokenizerIF::getToken() {
  LastChar = ' ';


  // Skip any whitespace and backspace.  
  while (LastChar==32 || LastChar==9 || LastChar==13) {
    LastChar = get();
  }
  
    

  if (LastChar=='[')
  {
    LastChar = get();
    return '[';
  }

  if (LastChar=='\'') {
    LastChar = get();

    if (LastChar == '\\') {              // escape sequence
        LastChar = get();
        switch (LastChar) {
            case 'n': NumVal = '\n'; break;
            case 't': NumVal = '\t'; break;
            case '0': NumVal = '\0'; break;
            case 'r': NumVal = '\r'; break;
            case '\\': NumVal = '\\'; break;
            case '\'': NumVal = '\''; break;
            default:
                std::cout << line << " - Unknown escape sequence\n";
        }
    } else {
        NumVal = LastChar;               // normal char
    }

    LastChar = get();

    if (LastChar!='\'')
        std::cout << line << " - Could not find matching \'\n";

    LastChar = get();
    return if_tok_char;
}

  if (LastChar=='"')
  {

    LastChar = get();
    if (LastChar=='"') {
      IdentifierStr = "";
      LastChar = get();
      return if_tok_str;
    }
    IdentifierStr = LastChar;

    while (true)
    {
      LastChar = get();
      if(LastChar=='"')
        break;
      IdentifierStr += LastChar;
    }
    LastChar = get();
    
    return if_tok_str;
  }


  if(LastChar=='.') 
  {
    LastChar = get();
    return '.';
  }
  

  if (isalpha(LastChar) || LastChar=='_') { // identifier: [a-zA-Z][a-zA-Z0-9]*
    // std::cout << "got alpha " << LastChar<< ".\n";
    IdentifierStr = LastChar;
    bool name_ok=true;
    while(true)
    {
      LastChar = get();
      if (LastChar=='['||LastChar=='.')
        break;
      
      if(isalnum(LastChar) || LastChar=='_')
      {
        IdentifierStr += LastChar;
        continue;
      }        
      break;
    }
    return if_tok_identifier;
  }
  
  if (isdigit(LastChar)) { // Number: [-.]+[0-9.]+
    bool is_float=false;
    
    std::string NumStr;
    if (LastChar == '-') { // Check for optional minus sign
      NumStr += LastChar;
      LastChar = get();
    }
    do {
      if(LastChar=='.')
      {
        
        is_float=true;
      }
      NumStr += LastChar;
      LastChar = get();
    } while (isdigit(LastChar) || LastChar == '.');

    NumVal = strtod(NumStr.c_str(), nullptr);

    return (is_float) ? if_tok_number : if_tok_int;
  }

  if (LastChar == '#') {
    // Comment until end of line.
    do
    {
      LastChar = get();
      if (LastChar==10)
        line++;
    }
    while (LastChar != EOF && LastChar != '\n' && LastChar != 10 && LastChar != '\r');

    if (LastChar != EOF)
      return getToken();
  }

  // Check for end of file.  Don't eat the EOF.
  if (LastChar == EOF)
    return if_tok_eof;

  // Otherwise, just return the character as its ascii value.
  int ThisChar = LastChar;


  if (ThisChar==10) {
      LastChar = get();
      return if_tok_space;
  }
  
  if (ThisChar==10 || LastChar==9)
  {
    int seen_spaces=0;
    while(LastChar==10 || LastChar==9 || LastChar==32) {
      if(LastChar==10)
        line++;
      if(ThisChar==10)
      {
        LastSeenTabs = SeenTabs;
        SeenTabs = 0;
        seen_spaces = 0;
      }
      if (LastChar==9) {
        SeenTabs+=1;
      }
      if (LastChar==32)
        seen_spaces++;
      if (seen_spaces==3)
      {
        seen_spaces=0;
        SeenTabs+=1;
      }
      ThisChar = (int)LastChar;
      LastChar = get(); 
      // std::cout << "Line Feed post: " << LastChar  << ".\n";
    }
    if (ThisChar==10&&isalpha(LastChar))
        SeenTabs = 0;

    return if_tok_space;
  }


  LastChar = get();
  int otherChar = LastChar;


  if(ThisChar=='<' && LastChar=='-')
  {
    LastChar = get();
    return if_tok_arrow;
  }

  if (ThisChar=='=' && otherChar=='=')
  {
    LastChar = get();
    return if_tok_equal;
  }
  if (ThisChar=='!' && otherChar=='=')
  {
    LastChar = get();
    return if_tok_diff;
  }
  if (ThisChar=='>' && otherChar=='=')
  {
    LastChar = get();
    return if_tok_higher_eq;
  }
  if (ThisChar=='<' && otherChar=='=')
  {
    LastChar = get();
    return if_tok_minor_eq;
  }

  if((ThisChar=='/')&&(otherChar == '/')){
    LastChar = get();
    return if_tok_int_div;
  }

  return ThisChar;
}

// int get_tokenPrecedence() {
//   if (CurTok==tok_space)
//   {
//     // if (CurTok==10)
//     //   LineCounter++;
//     return 1;
//   }


//   if (BinopPrecedence.find(CurTok) == BinopPrecedence.end()) // if not found
//     return -1;

//   // Make sure it's a declared binop.
//   int TokPrec = BinopPrecedence[CurTok];
//   if (TokPrec <= 0)
//     return -1;
//   return TokPrec;
// }
