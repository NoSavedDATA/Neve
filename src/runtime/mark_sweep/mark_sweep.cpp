#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <thread>
#include <unordered_map>

#include "../compiler_frontend/global_vars.h"
#include "../compiler_frontend/logging_v.h"
#include "../clean_up/clean_up.h"
#include "../data_types/list.h"
#include "../mangler/scope_struct.h"
#include "../pool/pool.h"
#include "include.h"




GC_span_traits::GC_span_traits(int obj_size) : obj_size(obj_size) {
    while (N<32&&pages<4) {
        pages++;
        N = (GC_page_size*pages) / obj_size;
        // std::cout << "N: " << N << ", pages: " << pages << ", obj size: " << obj_size << ".\n";
    }
    size = (GC_page_size*pages);

    if ((float)((GC_page_size*pages)/obj_size) != ((GC_page_size*pages)/(float)obj_size))
        N-=1;
}

GC_Span::GC_Span(GC_Arena *arena, GC_span_traits *traits, uint64_t gc_mark_bit) : arena(arena), traits(traits) {
    // Get Span address
    span_address = static_cast<char*>(arena->arena) + arena->size_allocated;
    arena_offset = arena->size_allocated;
    char *span_address_c = static_cast<char*>(span_address);
    arena->size_allocated += traits->size;

    // cur_free = span_address_c;
    // end = span_address_c + arena->size_allocated;

    elem_size = traits->obj_size;
    N = traits->N;

    // Set arena page idx
    for (int i=0; i<traits->pages; ++i) {
        arena->page_to_span[arena->pages_allocated] = this;
        arena->pages_allocated++;
    }

    // Get & initialize mark-bits
    uint64_t mask = gc_mark_bit ? 0ULL : ~0ULL;
    words = (traits->N + 63) / 64;
    // mark_bits = (uint64_t*)malloc(words*sizeof(uint64_t));
    mark_bits = new std::atomic<uint64_t>[words];
    alloc_bits = (uint64_t*)malloc(words*sizeof(uint64_t));
    for (int i=0; i<words; ++i) {
       alloc_bits[i] = 0ULL;
       mark_bits[i].store(mask, std::memory_order_release); 
    }

    // Protect unexisting slots
    for (int i=N; i<((N+63)/64)*64; ++i)
        set_1(alloc_bits, i, 1ULL);
    
    // Initialize type-metadata
    int types_per_word = 64 / 16;
    type_words = (traits->N + types_per_word-1) / types_per_word;
    type_metadata = new std::atomic<uint64_t>[type_words];
    for (int i=0; i<type_words; ++i)
       type_metadata[i].store(0ULL, std::memory_order_release); 
}

GC_Arena::GC_Arena(int tid, int size=GC_arena_size) {
    arena = aligned_alloc(8192, size);
    // arena = aligned_alloc(64u, arena_size);
    arena_base_addr[tid] = static_cast<char*>(arena);
}

GC::GC(int tid) {
    arena = new GC_Arena(tid);
    arena->gc = this;
}

WorkList::WorkList(GC_Node node) : node(node) {};

void GC_Observer(Scope_Struct *scope_struct) {

    GC *gc = scope_struct->gc;
    int sweeps = 0;

    auto next = std::chrono::steady_clock::now() + std::chrono::microseconds(5000);
 
    while (scope_struct->alive) {

        std::unique_lock<std::mutex> lock(scope_struct->mtx);

        scope_struct->cv.wait_until(
            lock,
            next,
            [&] { return !scope_struct->alive || scope_struct->force_sweep; }
        );

        // consume the signal
        bool forced = scope_struct->force_sweep;
        scope_struct->force_sweep = false;

        lock.unlock();

        if (!scope_struct->alive) break;
    

        int size = gc->size_occupied;
        // if (true) {
        if (gc->size_occupied>=gc->next_clean) {
            sweeps++;
            gc->Sweep(scope_struct);
            gc->next_clean = std::min(gc->next_clean*2, 8UL<<20);
            // gc->next_clean = std::min(size*2, 8<<20);
        }
        gc->retire_clean();
        next = std::chrono::steady_clock::now() + std::chrono::microseconds(5000);
    }
    std::cout << "sweeps " << sweeps << "\n";
}
extern "C" float psweep(Scope_Struct *scope_struct) {
    {
        std::lock_guard<std::mutex> lock(scope_struct->mtx);
        scope_struct->force_sweep = true;
    }
    scope_struct->cv.notify_one();
    return 0;
}

extern "C" float join_gc(Scope_Struct *scope_struct) {
    scope_struct->alive = false;
    if (scope_struct->gc_thread.joinable()) {
        scope_struct->gc_thread.join();
    }
    scope_struct->alive = true;
    scope_struct->joined = true;
    return 0;
}

extern "C" float sweep(Scope_Struct *scope_struct) {
    scope_struct->gc->Sweep(scope_struct);
    return 0;
}


// bool concurrent=false;
bool concurrent=true;

extern "C" void scope_struct_Join_GC(Scope_Struct *scope_struct) {
    if(concurrent && scope_struct->alive&&!scope_struct->joined) {
        scope_struct->alive = false;
        if (scope_struct->gc_thread.joinable()) {
            scope_struct->gc_thread.join();
        }
    }
}


extern "C" void scope_struct_Alloc_GC(Scope_Struct *scope_struct) {
    GC *gc = new GC(scope_struct->thread_id);
    scope_struct->gc = gc;
    if(concurrent)
        scope_struct->gc_thread = std::thread(GC_Observer, scope_struct);
}



extern "C" float GC_print(Scope_Struct *scope_struct) {
    std::cout << "print gc" << "\n";
    GC *gc = scope_struct->gc;
    std::cout << scope_struct;
    std::cout << " has gc " << gc << "\n";
    gc->Print();
    return 0;
}





// //---------------------------------------------------------//

GC_Node::GC_Node(void *ptr, uint16_t type) : ptr(ptr), type(type) {}
GC_Node::GC_Node() {}




void GC::DoubleSize(Scope_Struct *ctx) {
    // Not working
    std::unique_lock<std::mutex> lock(arena->sweep_mtx);
    std::cout << "DOUBLE ARENA SIZE" << "\n";

    int prev_size = arena->arena_size;
    arena->arena_size*=2;
    void *new_arena = aligned_alloc(8192, arena->arena_size);

    memcpy(new_arena, arena->arena, prev_size);

    for (int span_group=0; span_group<GC_obj_sizes; span_group++) {
        for (const auto &span : arena->Spans[span_group]) {
            GC_span_traits *traits = span->traits;
            span->span_address = (char*)new_arena + span->arena_offset;
        }  
    }
    // free(arena->arena);
    arena->arena = new_arena;
}

