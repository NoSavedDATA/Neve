#pragma once

#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include "../mark_sweep/include.h" 

struct GC;
struct GC_span;

constexpr int ContextStackSize = 4096;
constexpr int PrintBufferSize = 16384;

struct Scope_Struct { 
    int code_line=0;
    int thread_id=0;
    void *pointers_stack[ContextStackSize];
    int stack_top=0;
    void *object_ptr = nullptr;
    GC *gc=nullptr;

    char print_buffer[PrintBufferSize];
    int tN=0;

    std::vector<GC_Node> root_nodes;

    Scope_Struct *previous_scope=nullptr;

    char *first_arg = nullptr;
    char *scope = nullptr;
    char *function_name = nullptr;


    Thread_State *spans;

    int has_grad=1;

    int asyncs_count = 0;

    bool is_at_return = false, alive=true;

    std::thread gc_thread;
    std::mutex mtx;
    std::condition_variable cv;
    bool force_sweep = false, joined=false;



    // std::map<std::string, std::vector<std::vector<std::string>>> debug_map;


    Scope_Struct();

    void Set_First_Arg(char *);
    void Set_Scope(char *);
    void Set_Function_Name(char *);
    void Set_Thread_Id(int);
    void Set_Has_Grad(int);

    void Copy(Scope_Struct *);

    void Print();
    void Print_Stack();

    inline void *Allocate(int size, int type_id) {
        void *ret = gc->Allocate(this, spans, size, type_id, thread_id); 
        return ret;
    }
    inline void stw_wait() {
        spans->stw_wait();
    }
};


void alloc_gc_vspace(Scope_Struct *scope_struct, int size);

extern std::map<std::string, Scope_Struct *> NamedScopeStructs;
