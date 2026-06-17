#include <atomic>
#include <cstring>
#include <mutex>
#include <set>
#include <thread>
#include <unistd.h>
#include <unordered_set>

#include "../char_pool/include.h"
#include "../codegen/string.h"
#include "../codegen/time.h"
#include "../compiler_frontend/global_vars.h"
#include "../mark_sweep/include.h" 
#include "../pool/include.h" 
#include "../threads/include.h"

#include "scope_struct.h"


extern "C" void print_stack1(Scope_Struct *scope_struct) {
    std::cout << "stack[0]: " << scope_struct->pointers_stack[0] << ".\n";
    std::cout << "stack[1]: " << scope_struct->pointers_stack[1] << ".\n";
}

extern "C" void print_stack(Scope_Struct *scope_struct, void *stack) {
    std::cout << "stack[0]: " << scope_struct->pointers_stack[0] << ".\n";
    std::cout << "stack[1]: " << scope_struct->pointers_stack[1] << ".\n";
    std::cout << "stack top: " << stack << ".\n";
}





Scope_Struct::Scope_Struct() {
}


std::map<std::string, Scope_Struct *> NamedScopeStructs;


void Scope_Struct::Set_Thread_Id(int thread_id) {
    this->thread_id = thread_id;
}
void Scope_Struct::Set_Has_Grad(int has_grad) {
    this->has_grad = has_grad;
}
void Scope_Struct::Copy(Scope_Struct *scope_to_copy) {
    object_ptr = scope_to_copy->object_ptr;

    thread_id = scope_to_copy->thread_id;
    has_grad = scope_to_copy->has_grad;
    code_line = scope_to_copy->code_line;

    asyncs_count = scope_to_copy->asyncs_count;
    
    previous_scope = scope_to_copy;

    gc = scope_to_copy->gc;
    tN = scope_to_copy->asyncs_count;
}



void Scope_Struct::Print() {
    std::cout << "Scope struct:" << "\n\tThread id: " << thread_id << ".\n\n";
}

void Scope_Struct::Print_Stack() {
    std::cout << "has " << stack_top << " stack items.\n";
    for (int i=0; i<stack_top; ++i) {
        std::cout << i << ": " << pointers_stack[i] << "\n";        
    }
}



extern "C" float scope_struct_spec(Scope_Struct *scope_struct) {
    std::cout << "scope_struct: " << scope_struct << ".\n";
    return 0;
}

extern "C" void set_scope_line(Scope_Struct *scope_struct, int line) {
    scope_struct->code_line = line;
}


extern "C" Scope_Struct *scope_struct_CreateFirst() {
    Scope_Struct *scope_struct = new Scope_Struct();
    return scope_struct;
}
extern "C" Scope_Struct *scope_struct_Create() {
    Scope_Struct *scope_struct = new Scope_Struct();
    // std::cout << "Create scope " << scope_struct << ".\n";
    return scope_struct;
}







extern "C" void set_scope_thread_id(Scope_Struct *scope_struct, int thread_id) {
    // std::cout << "set_scope_thread_id: " << thread_id << ".\n";
    scope_struct->Set_Thread_Id(thread_id);
}



extern "C" int get_scope_thread_id(Scope_Struct *scope_struct) {
    // std::cout << "get_scope_thread_id " << scope_struct->thread_id << ".\n";
    return scope_struct->thread_id;
}


std::unordered_set<int> assigned_ids;
std::mutex id_mutex;
static std::atomic<int> next_thread_id(1);
std::atomic<int> next_id(1);

extern "C" float scope_struct_Reset_Threads(Scope_Struct *scope_struct) {
    // std::cout << "-----------------RESET THREADS--------------------------"  << ".\n";
    assigned_ids.clear();    
    next_id = 1;
    return 0;
}

extern "C" float scope_struct_Increment_Thread(Scope_Struct *scope_struct) {
    static thread_local bool has_id = false;
    static thread_local int thread_id = 0;
    if (!has_id) {
        std::lock_guard<std::mutex> lock(id_mutex);
        thread_id = next_id++;
        // Handle wrap-around
        if (thread_id < 1) {
            next_id = 1;
            thread_id = 1;
        }
        assigned_ids.insert(thread_id);
        has_id = true;
    }
    
    scope_struct->thread_id = thread_id;
    auto state = new Thread_State(scope_struct);
    scope_struct->spans = state;
    scope_struct->gc->states[thread_id] = state;
    return 0;
}

extern "C" void scope_struct_Print(Scope_Struct *scope_struct) {
    std::cout << "Printing scope:" << ".\n";
    scope_struct->Print();
}




extern "C" void scope_struct_Save_for_Async(Scope_Struct *scope_struct, char *fn_name) {
    NamedScopeStructs[fn_name] = scope_struct;
}

extern "C" void *scope_struct_Load_for_Async(int uniques_count, char *fn_name) {
    Scope_Struct *scope_struct = NamedScopeStructs[fn_name];

    Scope_Struct *scope_struct_copy = new Scope_Struct();
    scope_struct_copy->Copy(scope_struct);

    for(int i=0; i<uniques_count; ++i)
        scope_struct_copy->pointers_stack[i] = scope_struct->pointers_stack[i];
    scope_struct_copy->stack_top = uniques_count;

    return scope_struct_copy;
}

extern "C" void scope_struct_Store_Asyncs_Count(Scope_Struct *scope_struct, int asyncs_count) {
    scope_struct->asyncs_count = asyncs_count;
}


extern "C" void scope_struct_Get_Async_Scope(Scope_Struct *scope_struct, int thread_id, int has_grad) {
    scope_struct->thread_id = thread_id;
}



extern "C" float ctx_print_buffer(Scope_Struct *ctx, int offset) {
    // write(1, ctx->print_buffer, offset);
    return 0;
}





inline void delete_scope(Scope_Struct *scope_struct) {
    delete scope_struct;
}


void alloc_gc_vspace(Scope_Struct *scope_struct, int size) {
    scope_struct->gc->size_occupied += size;
}

extern "C" float scope_struct_print(Scope_Struct *scope_struct) {
    std::cout << "scope_struct: " << scope_struct << "\n";
    std::cout << "obj: " << scope_struct->object_ptr << "\n";

    std::cout << "\n";
    return 0;
}

extern "C" void scope_struct_Sweep(Scope_Struct *scope_struct) {
    GC *gc = scope_struct->gc;
    // std::cout << "sweep: " << scope_struct << " - / - " << gc << ".\n";
    // scope_struct->Print_Stack();
    // std::cout << "sweep check: " << gc->allocations << "/" << gc->size_occupied << ".\n";
    gc->Sweep(scope_struct);
}

extern "C" void scope_struct_Delete(Scope_Struct *scope_struct) {
    // Called by threads exiting
    
    // int tid = scope_struct->thread_id;
    // scope_struct->gc->states[tid] = nullptr;
    // free(scope_struct->spans);
    // free(scope_struct);
}

