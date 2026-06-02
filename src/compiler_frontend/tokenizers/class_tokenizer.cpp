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

#include "../../runtime/common/extension_functions.h"
#include "../../runtime/compiler_frontend/logging_v.h"
#include "class_tokenizer.h"




namespace fs = std::filesystem;





//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//



TokenizerClass::TokenizerClass(std::string file) : TokenizerIF(file) {
}



/// get_token - Return the next token from standard input.
int TokenizerClass::getToken() {
  // LastChar = ' ';

  // Skip any whitespace and backspace.  
  int seenspaces=0;
  while (LastChar==32 || LastChar==9 || LastChar==13) {
    if(LastChar==9)
        SeenTabs++;
    if (LastChar==32)
        seenspaces++;
    if (seenspaces==3) {
        seenspaces=0; 
        SeenTabs++;
    }
    LastChar = get();
  }


  if(LastChar=='.') {
    LastChar = get();
    return '.';
  }

  if(LastChar=='\"') {
    LastChar = get();
    while(LastChar!='\"')
        LastChar = get();
    LastChar = get();
    return class_tok_string;
  }
    
  if(LastChar=='#') {
    while(LastChar!=10) {
        if (LastChar==EOF)
            return class_tok_eof;
        LastChar = get();
    }
    LastChar = get();
    return class_tok_commentary;
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
    if (IdentifierStr=="class")
        return class_tok_class;
    if (IdentifierStr=="import")
        return class_tok_import;
    if (IdentifierStr=="ctor")
        return class_tok_ctor;
    if (IdentifierStr=="def")
        return class_tok_def;
    return class_tok_identifier;
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
        is_float=true;

      NumStr += LastChar;
      LastChar = get();
    } while (isdigit(LastChar) || LastChar == '.');

    NumVal = strtod(NumStr.c_str(), nullptr);

    return (is_float) ? class_tok_float : class_tok_int;
  }

  

  // Check for end of file.  Don't eat the EOF.
  if (LastChar == EOF)
    return class_tok_eof;

  // Otherwise, just return the character as its ascii value.
  int ThisChar = LastChar;


  if (ThisChar==10) {
      LastChar = get();
      SeenTabs=0;
      return class_tok_space;
  }
  LastChar = get();


  return ThisChar;
}
