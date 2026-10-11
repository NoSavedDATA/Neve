#pragma once

#include <iostream>


#include "../compiler_frontend/global_vars.h"
#include "../compiler_frontend/logging_v.h"
#include "../mangler/scope_struct.h"
#include "../mark_sweep/include.h"

#include "pool.h"

// extern std::unordered_map<int, std::unordered_map<int, std::vector<void*>>> memory_cache;
extern std::array<std::array<std::vector<void*>, 40>, 100> memory_cache;



template<typename T>
T *allocate(Scope_Struct *scope_struct, int size, std::string type) {
    if (size==0)
        return nullptr;



    auto it = data_name_to_type().find(type);
    if (it==data_name_to_type().end())
        LogErrorC(-1, std::string("Type ") + type + " not implemented.");
    uint16_t type_id = it->second;


    int alloc_size = size*sizeof(T);
    // void *v_ptr = malloc(alloc_size);
    void *v_ptr = scope_struct->Allocate(alloc_size, type_id);
    
    scope_struct->gc->size_occupied += alloc_size;
    scope_struct->gc->allocations++;
    
    return static_cast<T*>(v_ptr);
}

template<typename T>
T *allocate(Scope_Struct *scope_struct, int size, uint16_t type_id) {
    if (size==0)
        return nullptr;


    int alloc_size = size*sizeof(T);
    // void *v_ptr = malloc(alloc_size);
    void *v_ptr = scope_struct->Allocate(alloc_size, type_id);
    
    scope_struct->gc->size_occupied += alloc_size;
    scope_struct->gc->allocations++;
    
    return static_cast<T*>(v_ptr);
}

template<typename T>
T *newT(Scope_Struct *scope_struct, std::string type) {
    T *ptr;

    if(scope_struct!=nullptr) {
        auto it = data_name_to_type().find(type);
        if (it==data_name_to_type().end())
            LogErrorC(-1, std::string("Type ") + type + " not implemented.");
        uint16_t type_id = it->second;
        // if (type_id==104) {
        //     std::cout << "GOT MAP" << "\n";
        //     std::exit(0);
        // }

        // if (type_id==102)
        //     std::cout << "NEWT " << "\n";
        void *v_ptr = scope_struct->Allocate(sizeof(T), type_id);
        ptr = new (v_ptr) T();

        // std::cout << " -- newT of --> " << type << "/" << type_id << " - / - " << ptr << ".\n";

        int size = sizeof(T);
        scope_struct->gc->size_occupied += size;
        scope_struct->gc->allocations++;

        // scope_struct->gc.pointer_nodes.push_back(GC_Node(static_cast<void *>(ptr), type));
   } else {
        std::cout << "allocate non-pool for " << type << ".\n";
        ptr = new T();
   }

    return ptr;
}


inline uint32_t round_up_pow2(uint32_t v) {
    if (v <= 1) return 1;
    return 1u << (32 - __builtin_clz(v - 1));
}
inline uint32_t round_up_pow2_exp(uint32_t v) {
    if (v <= 1) return 0;          // 2^0 = 1
    return 32 - __builtin_clz(v - 1);
}

inline void *cache_pop(int size, int tid) {
    // int exp = round_up_pow2_exp(size);
    // auto& bucket = memory_cache[tid][exp];
    // if (bucket.empty())
        return malloc(round_up_pow2(size));
    // void* ptr = bucket.back();
    // bucket.pop_back();
    // return ptr;
}

inline void cache_push(void* ptr, int size, int tid) {
    free(ptr);
    // int exp = round_up_pow2_exp(size);
    // auto& bucket = memory_cache[tid][exp];
    // bucket.push_back(ptr);
}
