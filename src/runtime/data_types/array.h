#pragma once
#include <string>
#include "../mangler/scope_struct.h"

struct DT_array_retire {
    DT_array_retire *next;
    void *data;
    int size, tid;
    DT_array_retire(void *data, int, int);
};

class DT_array {
    public:
		int virtual_size, size, elem_size;
		void *data=nullptr;
        uint16_t type=0;

    DT_array();
    void New(Scope_Struct*,int, int, int, uint16_t, int memTy=0);
    void New(Scope_Struct*,int, int, uint16_t);
};


void array_Clean_Up(void *data_ptr, int);

extern "C" void array_double_size(Scope_Struct *scope_struct, DT_array *vec);

extern "C" int hash_array_int(Scope_Struct *ctx, DT_array *arr);
