#pragma once

#include <cstdint>
#include <map>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <string>
#include <unordered_map>
#include <vector>

#include "../compiler_frontend/global_vars.h"
#include "../compiler_frontend/logging_v.h"
#include "../clean_up/clean_up.h"
#include "../data_types/array.h"
#include "../data_types/map.h"
#include "../data_types/list.h"
#include "../mangler/scope_struct.h"
#include "../pool/pool.h"
#include "include.h"

const int word_bits=64;

const int GC_page_size=8192;

constexpr size_t sweep_after_alloc = 32 << 20;
//constexpr size_t GC_arena_size = 1024 << 20;
constexpr size_t GC_arena_size = 256 << 20;
// constexpr size_t GC_arena_size = 64 << 20;

constexpr size_t pages_per_arena = GC_arena_size / GC_page_size;

const int GC_obj_sizes=16;
const int GC_max_object_size = 32768;
extern int gc_sizes[GC_obj_sizes];

extern std::array<char *, 100> arena_base_addr;


constexpr size_t GC_ALIGN = 8; // 8-byte granularity
constexpr size_t GC_N = GC_max_object_size / GC_ALIGN;

extern uint16_t GC_size_to_class[GC_N+1];
extern uint16_t GC_size_to_c[GC_N+1];

struct DT_array_retire;
struct Scope_Struct;
struct GC_span_traits;
struct GC_Arena;
struct GC_Node;
struct GC_Span;
struct GC;



struct Thread_State {
    std::array<GC_Span*, GC_obj_sizes> current_span{};
    bool stw=false; //atomic
    Scope_Struct *ctx;
    Thread_State(Scope_Struct*);
};

extern std::array<GC_span_traits*, GC_obj_sizes> GC_span_traits_vec;

struct GC_Node{
    void *ptr;
    uint16_t type;
    GC_Node(void *, uint16_t);
    GC_Node();
};


struct GC_span_traits {
    int pages=0, N=0, size, obj_size;
    GC_span_traits(int);
};

struct GC_Span {
    GC_span_traits *traits;
    GC_Arena *arena;
    void *span_address;
    // char *cur_free, *end;
    GC_Span *next_span=nullptr;
    bool sweeped=true;

    int words, type_words, free_idx=0, elem_size, N, gen=0, arena_offset;

    // Interpretate type_metadata as int12
    // uint64_t *mark_bits, *alloc_bits, *type_metadata;
    uint64_t *alloc_bits;
    std::atomic<uint64_t> *mark_bits, *type_metadata;
    
    GC_Span(GC_Arena *, GC_span_traits *, uint64_t);
    int Sweep(int, uint64_t);
    inline void *Allocate(uint16_t type_id, uint64_t gc_mark_bit, int tid) {
        if(!sweeped)
            Sweep(tid, gc_mark_bit);
        if(free_idx<N) {
            set_1_atomic(mark_bits, free_idx, gc_mark_bit);
            void *ret_ptr = static_cast<char*>(span_address) + elem_size*free_idx;
            alloc_bits[free_idx >> 6] |= (1ULL << (free_idx & 63));
            set_16_r12(type_metadata, free_idx, type_id);
            free_idx++;
            return ret_ptr;
        }
        for (int w=0; w<words; ++w) {
            uint64_t bits = alloc_bits[w] ^ ~0ULL;
            if (bits) {
                int idx = (w<<6) + __builtin_ctzll(bits);
                set_1_atomic(mark_bits, idx, gc_mark_bit);
                void *ret_ptr = static_cast<char*>(span_address) + elem_size*idx;
                alloc_bits[idx >> 6] |= (1ULL << (idx & 63));
                set_16_r12(type_metadata, idx, type_id);
                return ret_ptr;
            }
        }
        return nullptr;
    }
};


inline bool Check_Arena_Size_Ok(const int arena_size, const int size_allocated) {
    if(size_allocated>arena_size)
        return false; 
    return true;
}

struct WorkList {
    GC_Node node;
    WorkList *next=nullptr;
    WorkList(GC_Node);
};

struct GC_Arena {
    // Get an arena of 64MB, and set pages size to 8 KB
    const int page=GC_page_size;
    // const int arena_size=65536, page=8192;
    int arena_size=GC_arena_size, size_allocated=0,pages_allocated=0,gen=0;
    void *arena, *metadata;
    GC *gc;

    std::mutex sweep_mtx, span_mtx;

    std::array<std::vector<GC_Span*>, GC_obj_sizes> Spans;
    std::array<GC_Span*, GC_obj_sizes> cur_span{};
    std::array<GC_Span*, pages_per_arena> page_to_span;
    WorkList *topw=nullptr;
    std::atomic<WorkList*> mutatorw{nullptr};

    GC_Arena(int, int);

    inline void mutator_push(void *ptr, uint16_t type_id) {
        WorkList *old_head, *node = new WorkList(GC_Node(ptr, type_id));
        do {
            old_head = mutatorw.load(std::memory_order_relaxed);
            node->next = old_head;
        } while (!mutatorw.compare_exchange_weak(
            old_head,
            node,
            std::memory_order_release,
        std::memory_order_relaxed));
    }

    inline void walkw() {
        WorkList *old = topw;
        topw = topw->next;
        delete old;
    }
    inline void worklist_push(void *ptr, uint16_t type_id) {
        WorkList *node = new WorkList(GC_Node(ptr, type_id));
        node->next = topw;
        topw = node;
    }

    inline void* Allocate(Thread_State *spans, int size_class, uint16_t type_id, int tid, uint64_t gc_mark_bit) {
        std::unique_lock<std::mutex> lock(sweep_mtx);

        GC_span_traits* traits = GC_span_traits_vec[size_class];
        GC_Span* span = spans->current_span[size_class];
        GC_Span *prev_span = span;

        // FAST PATH
        if (span != nullptr) {
            void* ptr = span->Allocate(type_id, gc_mark_bit, tid);
            if (ptr != nullptr)
                return ptr;

            // {
            //     std::unique_lock<std::mutex> lock(span_mtx);
                span = cur_span[size_class];
                while(span!=nullptr) { // only happens after resets
                    ptr = span->Allocate(type_id, gc_mark_bit, tid);
                    if (ptr!=nullptr) {
                        spans->current_span[size_class] = span;
                        cur_span[size_class] = span->next_span;
                        return ptr;
                    }
                    span = span->next_span; 
                }
            // }
        }


        // Need new span
        if (!Check_Arena_Size_Ok(arena_size, size_allocated + traits->size))
            return nullptr;

        span = new GC_Span(this, traits, gc_mark_bit);

        spans->current_span[size_class] = span;

        // {
        //     std::unique_lock<std::mutex> lock(span_mtx);
            Spans[size_class].push_back(span);
        // }

        return span->Allocate(type_id, gc_mark_bit, tid);
    }

    bool is_safe(void *node_ptr);
    bool get_is_marked(void *node_ptr, uint64_t mark_bit);
    bool mark_obj(void *node_ptr, uint16_t &type, uint64_t mark_bit);
    void gc_list(void *ptr, uint16_t root_type, uint64_t mark_bit);
    bool mark_worklist_pointers(Scope_Struct *scope_struct, uint64_t mark_bit);
    bool mark_worklist_pointers2(Scope_Struct *scope_struct, uint64_t mark_bit);
    void check_roots_worklist(Scope_Struct *scope_struct, uint64_t mark_bit);
    void Work(Scope_Struct *);
};


struct GC {
    int allocations=0;
    uint64_t size_occupied=0, mark_bit=1ULL;
    bool marking = false;
    uint64_t next_clean = 16<<10;
    GC_Arena *arena;
    DT_array_retire *retired_arr=nullptr;
    DT_node_retire *retired_nodes=nullptr;

    std::array<Thread_State*, 64> thread_states{};
    
    GC(int);
    void DoubleSize(Scope_Struct*);
    inline void Print(Thread_State *spans) {
        std::cout << "Arena addr: " << arena->arena << "\n";
        std::cout << "allocated: " << arena->size_allocated << "\n";
        for (int i=0; i<GC_obj_sizes; ++i) {
            std::cout << gc_sizes[i] << ": " << arena->Spans[i].size() << "\n";
            if (arena->Spans[i].size()>0) {
                if (spans->current_span[i])
                std::cout << "\t" << spans->current_span[i]->free_idx << " / " << spans->current_span[i]->N << "\n";
            }
        }
        std::cout << "\n";
    }
    inline void *Allocate(Scope_Struct *scope_struct, Thread_State *spans, int size, uint16_t type_id, int tid) {
        int obj_class = GC_size_to_c[(size+7)/8];

        if(size>GC_max_object_size) {
            LogErrorC(-1, "Allocated object of size " + std::to_string(size) + ", but the maximum supported object size is " + std::to_string(GC_max_object_size) + ".");
            return nullptr;
         }
        
        void *ptr = arena->Allocate(spans, obj_class, type_id, tid, mark_bit);
        if(ptr==nullptr) {
            Sweep(scope_struct);
            ptr = arena->Allocate(spans, obj_class, type_id, tid, mark_bit);
            if (ptr==nullptr) {
                LogErrorC(-1, "ARENA FULL.");
                Print(spans);
                std::exit(0);
                DoubleSize(scope_struct);
                return arena->Allocate(spans, obj_class, type_id, tid, mark_bit);
            }
        }
        return ptr;
    }


    void retire_nodes(DT_map_node **data, int tid);
    void retire_node(DT_map_node *data, int tid);
    void retire_arr(void *, int, int);
    void retire_clean();
    void Sweep(Scope_Struct *);
    void Worker(Scope_Struct *);
    void CleanUp_Unused(int);
};


//---------------------------------------------------------//






void protect_pool_addr(Scope_Struct *scope_struct, void *addr);
bool unprotect_pool_addr(Scope_Struct *scope_struct, void *addr);
