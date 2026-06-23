#include "../mangler/scope_struct.h"

extern "C" float print(Scope_Struct *scope_struct, char* str){
  printf("%s\n", str);
  return 0;
}


extern "C" void print_void_ptr(void *x) {
    std::cout << "--->GOT void*: " << x << ".\n";
}
extern "C" float print_void_ptrC(Scope_Struct *scope_struct, void *x) {
    std::cout << "--->GOT void*: " << x << ".\n";
    return 0;
}

extern "C" void print_bool(bool x) {
    // if(x!=0)
    std::cout << "GOT BOOL: " << x << ".\n";
}
extern "C" void print_int16(uint16_t x) {
    // if(x!=0)
    std::cout << "GOT INT: " << x << ".\n";
}
extern "C" int print_int(int x) {
    // if(x!=0)
    std::cout << "GOT INT: " << x << ".\n";
    return 0;
}
extern "C" void print_float(float x) {
    // if(x!=0)
    std::cout << "GOT FLOAT: " << x << ".\n";
}

extern "C" void print_int64(int64_t x) {
    std::cout << "GOT INT 64: " << x << ".\n";
}

extern "C" void print_uint64(uint64_t x) {
    std::cout << "GOT U INT 64: " << x << ".\n";
}
