#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <glob.h>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "../char_pool/include.h"
#include "../common/extension_functions.h"
#include "../codegen/random.h"
#include "../codegen/string.h"
#include "../compiler_frontend/include.h"
#include "../mangler/scope_struct.h"
#include "../pool/include.h" 
#include "include.h"


#ifdef _WIN32
#define strtok_r strtok_s
#endif


DT_str::DT_str() {}
DT_str::DT_str(char *str, int size) : str(str), size(size) {}

  

extern "C" char *str_Copy(Scope_Struct *scope_struct, char *str) {
  // std::cout << "Copying string: " << str << ".\n";
  char *ret = CopyString(scope_struct, str);
  return ret;
}


extern "C" bool str_eq(Scope_Struct *scope_struct, char *a, char *b, int l_size, int r_size) {
    return l_size == r_size &&
           std::memcmp(a, b, l_size) == 0;
}


extern "C" DT_str str_str_add(Scope_Struct *scope_struct, DT_str x, DT_str y) {
    int cat_size = x.size + y.size;
    char *ptr = (char*)allocate_pool(scope_struct, cat_size, 112);

    memcpy(ptr, x.str, x.size);
    memcpy(ptr + x.size, y.str, y.size);

    return DT_str(ptr, cat_size);
}


extern "C" float str_float(Scope_Struct *scope_struct, DT_str str) {
    char* p = str.str;
    char* end = str.str + str.size;

    bool neg = false;
    if (p < end && (*p == '-' || *p == '+')) {
        neg = (*p == '-');
        ++p;
    }

    float value = 0.0f;

    // integer part
    while (p < end && *p >= '0' && *p <= '9') {
        value = value * 10.0f + (*p - '0');
        ++p;
    }

    // fractional part
    if (p < end && *p == '.') {
        ++p;
        float frac = 1.0f;
        while (p < end && *p >= '0' && *p <= '9') {
            frac *= 0.1f;
            value += (*p - '0') * frac;
            ++p;
        }
    }

    // exponent
    if (p < end && (*p == 'e' || *p == 'E')) {
        ++p;
        bool exp_neg = false;
        if (p < end && (*p == '-' || *p == '+')) {
            exp_neg = (*p == '-');
            ++p;
        }

        int exp = 0;
        while (p < end && *p >= '0' && *p <= '9') {
            exp = exp * 10 + (*p - '0');
            ++p;
        }

        float scale = std::pow(10.0f, exp_neg ? -exp : exp);
        value *= scale;
    }

    return neg ? -value : value;
}



extern "C" char * str_int_add(Scope_Struct *scope_struct, char *lc, int rc)
{
  // std::cout << "Concat string and int fn" << ".\n";
  // std::cout << "Int is: " << rc << ".\n";
  // std::cout << "Concat: " << lc << " -- " << rc << ".\n";

  size_t length_lc = strlen(lc);

  // Convert the float to a string
  std::stringstream ss;
  ss << rc; // Adjust precision as needed
  std::string rc_str = ss.str();
  size_t length_rc = rc_str.length() + 1; // +1 for null terminator

  char *result_cstr = allocate<char>(scope_struct, length_lc+length_rc, "str");

  memcpy(result_cstr, lc, length_lc);
  memcpy(result_cstr + length_lc, rc_str.c_str(), length_rc);

  // std::cout << "cat str int " << result_cstr << ".\n";

  return result_cstr;
}



extern "C" char * str_float_add(Scope_Struct *scope_struct, char *lc, float rc)
{
  // std::cout << "Concat string and float fn" << ".\n";
  // std::cout << "Concat: " << lc << " -- " << rc << ".\n";

  size_t length_lc = strlen(lc);

  // Convert the float to a string
  std::stringstream ss;
  ss << std::fixed << std::setprecision(2) << rc; // Adjust precision as needed
  std::string rc_str = ss.str();
  size_t length_rc = rc_str.length() + 1; // +1 for null terminator

  char *result_cstr = allocate<char>(scope_struct, length_lc+length_rc, "str");

  memcpy(result_cstr, lc, length_lc);
  memcpy(result_cstr + length_lc, rc_str.c_str(), length_rc);


  return result_cstr;
}

extern "C" char * int_str_add(Scope_Struct *scope_struct, int lc, char *rc)
{   
  std::stringstream ss;
  ss << lc; // Adjust precision as needed
  std::string lc_str = ss.str();
  size_t length_lc = lc_str.length(); // +1 for null terminator

  
  size_t length_rc = strlen(rc) + 1; // +1 for null terminator
  
  char *result_cstr = allocate<char>(scope_struct, length_lc+length_rc, "str");

  
  memcpy(result_cstr, lc_str.c_str(), length_lc);
  memcpy(result_cstr + length_lc, rc, length_rc);


  return result_cstr;
}


extern "C" char * float_str_add(Scope_Struct *scope_struct, float lc, char *rc)
{  
  std::stringstream ss;
  ss << std::fixed << std::setprecision(2) << lc; // Adjust precision as needed
  std::string lc_str = ss.str();
  size_t length_lc = lc_str.length();
  
  size_t length_rc = strlen(rc) + 1; // +1 for null terminator

  char *result_cstr = allocate<char>(scope_struct, length_lc+length_rc, "str");
  
  memcpy(result_cstr, lc_str.c_str(), length_lc);
  memcpy(result_cstr + length_lc, rc, length_rc);

  return result_cstr;
}



extern "C" char * str_bool_add(Scope_Struct *scope_struct, char *lc, bool rc)
{
  size_t length_lc = strlen(lc);

  size_t length_rc;
  std::string r;
  if(rc) {
    length_rc = 5;
    r = "true";
  } else {
    length_rc = 6;
    r = "false";
  }

  char *result_cstr = allocate<char>(scope_struct, length_lc+length_rc, "str");

  memcpy(result_cstr, lc, length_lc);
  memcpy(result_cstr + length_lc, r.c_str(), length_rc);

  return result_cstr;
}

extern "C" char * bool_str_add(Scope_Struct *scope_struct, bool lc, char *rc)
{
  size_t length_lc;
  std::string l;
  if(rc) {
    length_lc = 4;
    l = "true";
  } else {
    length_lc = 5;
    l = "false";
  }

  size_t length_rc = strlen(rc)+1;

  char *result_cstr = allocate<char>(scope_struct, length_lc+length_rc, "str");

  memcpy(result_cstr, l.c_str(), length_lc);
  memcpy(result_cstr + length_lc, rc, length_rc);

  return result_cstr;
}









extern "C" float PrintStr(char* value){
  std::cout << "Str: " << value << "\n";
  return 0;
}






extern "C" char *cat_str_float(char *c, float v)
{

  std::string s = c;
  std::string tmp = std::to_string((int)v);

  s = s + c;

  return str_to_char(s);
}











// INDEX METHODS

extern "C" char *str_split_idx(Scope_Struct *scope_struct, char *self, char *pattern, int idx)
{
  // std::cout << "SLIPPINT " << self << ".\n";
  
  std::vector<char *> splits;
  char *input = (char*)malloc(strlen(self) + 1);
  memcpy(input, self, strlen(self) + 1);
  //strcpy(input, self);

  char *saveptr;
  char *token = strtok_r(input, pattern, &saveptr); // Get the first token

  while (token != nullptr) {
    splits.push_back(token);
    token = strtok_r(nullptr, pattern, &saveptr); // Get the next token
  }


  if(splits.size()<=1)
  {
    std::string _err = "\nFailed to split.";
    LogErrorC(scope_struct->code_line, _err);
    std::cout << "" << self << "\n";
    return nullptr;
  }

  if (idx < 0)
    idx = splits.size() + idx;
  
  // std::cout << "Spltting " << self << " with " << pattern <<" at ["<<idx<<"]:  " << splits[idx] << "\n";
 
  // Convert the retained token to a std::string
  char *result = CopyString(scope_struct, splits[idx]);


  free(input);

  return result;
}


// extern "C" DT_array *str_split(Scope_Struct *scope_struct, char *self, char *pattern) {
//     if (!self || !pattern) return nullptr;

//     size_t pat_len = strlen(pattern);
//     if (pat_len == 0) return nullptr;

//     std::vector<char*> tokens;

//     char *start = self;
//     char *pos;

//     while ((pos = strstr(start, pattern)) != nullptr) {
//         size_t len = pos - start;

//         char *out = allocate<char>(scope_struct, len + 1, "str");
//         memcpy(out, start, len);
//         out[len] = '\0';

//         tokens.push_back(out);

//         start = pos + pat_len;
//     }

//     // last token
//     if (*start) {
//         size_t len = strlen(start);
//         char *out = allocate<char>(scope_struct, len + 1, "str");
//         memcpy(out, start, len + 1);
//         tokens.push_back(out);
//     }

//     if (tokens.size() <= 1)
//         return nullptr;

//     DT_array *ret = newT<DT_array>(scope_struct, "array");
//     ret->New(tokens.size(), "str");

//     char **data = static_cast<char**>(ret->data);
//     for (size_t i = 0; i < tokens.size(); ++i)
//         data[i] = tokens[i];

//     return ret;
// }


extern "C" bool can_convert_to_float(Scope_Struct *scope_struct, char* str) {
    if (!str) return false;

    char* endptr;
    errno = 0;

    std::strtof(str, &endptr);

    // No conversion performed
    if (endptr == str)
        return false;

    // Overflow / underflow
    if (errno == ERANGE)
        return false;

    // Skip trailing whitespace
    while (std::isspace(static_cast<unsigned char>(*endptr)))
        ++endptr;

    // If anything remains, it's invalid
    return *endptr == '\0';
}

extern "C" float str_to_float(Scope_Struct *scope_struct, char *in_str)
{
  char *copied = (char*)malloc(strlen(in_str) + 1);
  strcpy(copied, in_str);
  char *end;

  float ret = std::strtof(copied, &end);

  free(copied);

  return ret;
}



extern "C" bool str_str_different(Scope_Struct scope_struct, char *l, char *r) {
  return strcmp(l, r);
}
extern "C" bool str_str_equal(Scope_Struct scope_struct, char *l, char *r) {
  return !strcmp(l, r);
}



extern "C" void str_Delete(char *in_str) {
  // std::cout << "str_Delete of " << in_str << ".\n";
  move_to_char_pool(strlen(in_str)+1, in_str, "free");
}



extern "C" char *readline(Scope_Struct *scope_struct) {
    std::string line;
    if (!std::getline(std::cin, line)) {
        // EOF or error
        char* buf = (char*)malloc(1);
        buf[0] = '\0';
        return buf;
    }

    char* buf = (char*)malloc(line.size() + 1);
    if (!buf) return nullptr;
    std::memcpy(buf, line.c_str(), line.size() + 1);
    return buf;
}



extern "C" DT_array *_glob_b_(Scope_Struct *scope_struct, char *pattern) {
  glob_t glob_result;
  bool ok=false;
  if (glob(pattern, GLOB_TILDE, NULL, &glob_result) == 0)
      ok=true;
  if (!ok) {
      std::string path = pattern;
    LogErrorC(scope_struct->code_line, "Glob failed to find files: " + path);
  }

  int size = glob_result.gl_pathc, elem_size = 16;
  DT_array *array = newT<DT_array>(scope_struct, "array");
  array->New(scope_struct, size, elem_size, scope_struct->thread_id, 100);

    
  DT_str *data = static_cast<DT_str*>(array->data);
  // std::cout << "glob has arr " << array << "\n";
  // std::cout << "glob has data " << data << "\n";

  for (size_t i = 0; i < size; ++i) {
    const char *src = glob_result.gl_pathv[i];
    size_t len = std::strlen(src)+1; //+1 for null terminator
    char *c_copy = allocate<char>(scope_struct, len, "charp");
    // char *c_copy = (char*)malloc(len);
    memcpy(c_copy, src, len);

    DT_str str_view = DT_str();
    str_view.str = c_copy;
    str_view.size = len;
    data[i] = str_view;
    // __atomic_store_n(&data[i], str_view, __ATOMIC_RELEASE);
  }

  return array;
}
