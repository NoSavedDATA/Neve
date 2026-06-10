#include <atomic>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <unordered_map>

#include "../../common/extension_functions.h"
#include "../../compiler_frontend/include.h"
#include "../../clean_up/clean_up.h"
#include "../../data_types/array.h"
#include "../../data_types/list.h"
#include "../../data_types/map.h"
#include "../../data_types/str_view.h"
#include "../../data_types/type_info.h"
#include "../../mangler/scope_struct.h"
#include "../../pool/include.h"
#include "../../pool/pool.h"
#include "../../threads/cas.h"
#include "../../threads/channels.h"
#include "../include.h"


int GC_Span::Sweep(int tid, uint64_t mark_bit) {
    uint64_t mark_mask = mark_bit ? ~0ULL : 0ULL;
    int free_slots=0;

    for (int w=0; w<words; ++w) {
        uint64_t bits = mark_bits[w].load(std::memory_order_acquire);
        bits = bits^mark_mask; 
        while (bits) {
            int idx = (w<<6) + __builtin_ctzll(bits);
            if(idx>=N)
                break;
            uint64_t set_mask = 1ULL << (idx&63);
            bits = bits & ~set_mask;
            if (get_1(alloc_bits, idx)) {
                uint16_t u_type = get_16_r12(type_metadata, idx);
                void *obj_addr = static_cast<char*>(span_address) + idx*elem_size;

                // if (u_type==104) continue;
                // if (u_type==102) continue;
                // if (u_type==103) continue;
                // if (elem_size==16)
                //     std::cout << "" << u_type << " | " << data_type_to_name()[u_type] << "\n";
                // if (u_type==103) continue;
                // if (elem_size==16)
                //     std::cout << "" << u_type << " | " << data_type_to_name()[u_type] << "\n";
                set_1(alloc_bits, idx, 0ULL); 
                // std::cout << "set0 to " << data_type_to_name()[u_type] << "|" << u_type << ", size: " << elem_size << ", addr: " << obj_addr << "\n";

                if (u_type==113)
                    std::cout << "IS TENSOR" << "\n";

                if(u_type==0||u_type==100||u_type==112||type_info[u_type]!=nullptr)
                    continue;

                std::string obj_type = data_type_to_name()[u_type]; 
                // std::cout << "CLEAN " << obj_type << "|" << u_type << " - " << obj_addr << ", size: " << elem_size << "\n";
                clean_up_functions[obj_type](obj_addr, tid);
            }
        }
        mark_bits[w].store(mark_mask, std::memory_order_release); // eager sweep

        bits = alloc_bits[w] ^ ~0ULL;
        while (bits) {
            int idx = (w<<6) + __builtin_ctzll(bits);
            free_slots++;
            if(idx>=N)
                break;
            uint64_t set_mask = 1ULL << (idx&63);
            bits = bits & ~set_mask;
        }
    }
    sweeped=true;
    if (free_slots>=N)
        free_idx = 0;
    return free_slots;
}



inline bool check_is_in_bounds(char *arena_addr, char *p) {
    bool in_bounds = (p>=arena_addr&&p<arena_addr+GC_arena_size);
    return in_bounds;
}


bool GC_Arena::is_safe(void *node_ptr) {
    if (!node_ptr)
        return false;
    
    char *arena_addr = (char*)arena;
    char *p = static_cast<char*>(node_ptr);
    
    if (!check_is_in_bounds(arena_addr, p))
        return false;

    return true;
}

bool GC_Arena::get_is_marked(void *node_ptr, uint64_t mark_bit) {
        if (!node_ptr)
            return true;
        
        char *arena_addr = (char*)arena;
        char *p = static_cast<char*>(node_ptr);
        
        if (!check_is_in_bounds(arena_addr, p))
            return true;
        long arena_offset = p - arena_addr;
        int page  =  (arena_offset / GC_page_size) % pages_per_arena;
 
        GC_Span *span = page_to_span[page];

        long obj_idx = (p - static_cast<char*>(span->span_address)) / span->traits->obj_size;

        return get_1_atomic(span->mark_bits, obj_idx) == mark_bit;
}

bool GC_Arena::mark_obj(void *node_ptr, uint16_t &type, uint64_t mark_bit) {
        if (!node_ptr)
            return false;
        
        char *arena_addr = (char*)arena;
        char *p = static_cast<char*>(node_ptr);
        
        if (!check_is_in_bounds(arena_addr, p))
            return false;
        
        long arena_offset = p - arena_addr;
        int page  =  (arena_offset / GC_page_size) % pages_per_arena;
 
        GC_Span *span = page_to_span[page];

        long obj_idx = (p - static_cast<char*>(span->span_address)) / span->traits->obj_size;

        
        type = get_16_r12(span->type_metadata, obj_idx);
        set_1_atomic(span->mark_bits, obj_idx, mark_bit);

        return true;
}

void GC_Arena::gc_list(void *ptr, uint16_t root_type, uint64_t mark_bit) {
    if (!is_safe(ptr))
        return;
    uint16_t type16;
    if (root_type==108) {
        Channel *ch = (Channel*)ptr;
        // std::cout << "gc_list of " << root_type << " -- " << data_type_to_name()[root_type] << " -- "  << ptr << "\n";
        // std::cout << "its array is " << ch->data << "\n";
        if (ch->type>100) {
            std::cout << "todo: non primary channel (gc_list - mark phase)" << "\n";
            std::exit(0);
        }
        // bool init = __atomic_load_n(&ch->init, __ATOMIC_RELEASE);
        return;
    }
    if (root_type==uint16_t{102}) { // array 
        DT_array *array = static_cast<DT_array*>(ptr);
        void *datap = __atomic_load_n(&array->data, __ATOMIC_ACQUIRE);
        void **data = (void**)datap;
        int size = __atomic_load_n(&array->virtual_size, __ATOMIC_ACQUIRE);
        int ssize = __atomic_load_n(&array->size, __ATOMIC_ACQUIRE);
        uint16_t type = __atomic_load_n(&array->type, __ATOMIC_ACQUIRE);
        // std::cout << "arr type: " << type << "\n";
        // std::cout << "arr ptr: " << datap << "\n";
        if (!data)
            return;
        if(type<100) // is primary
            return;

        if (type==100) { // DT_str
            char *base = (char*)data;
            for (int i=0; i<size; ++i) {
                void *elem = __atomic_load_n((void**)(base + i*16), __ATOMIC_ACQUIRE);
                if(!get_is_marked(elem, mark_bit))
                    worklist_push(elem, type);
                
            }
        } else { // general pointer
            for (int i=0; i<size; ++i) {
                void *elem = __atomic_load_n(&data[i], __ATOMIC_ACQUIRE);
                if(!get_is_marked(elem, mark_bit))
                    worklist_push(elem, type);
                
            }
        }
    }
    if (root_type==uint16_t{104}) { // map 
        DT_map *map = static_cast<DT_map*>(ptr);

        uint16_t ktype = data_name_to_type()[map->key_type];
        uint16_t vtype = data_name_to_type()[map->val_type];
        bool mark_key = ktype>100; //not primary
        bool mark_val = vtype>100;
        int map_capacity = __atomic_load_n(&map->capacity,__ATOMIC_ACQUIRE);
        DT_map_node **nodes = __atomic_load_n(&map->nodes,__ATOMIC_ACQUIRE);


        // std::cout << "mark map " << ptr << " -- " << ktype << " - " << vtype << "\n";
        for (int i=0; i<map_capacity; ++i) {
            // DT_map_node *node = __atomic_load_n(&map->nodes[i],__ATOMIC_ACQUIRE);
            DT_map_node *node = __atomic_load_n(&nodes[i],__ATOMIC_ACQUIRE);
            while (node!=nullptr) {
                if (mark_key) {
                    void *key = __atomic_load_n(&node->key,__ATOMIC_ACQUIRE);
                    mark_obj(key, type16, mark_bit);
                    if (ktype==102||ktype==104)
                        gc_list(key, ktype, mark_bit);
                    else
                        worklist_push(key, ktype);
                }
                if(mark_val) {
                    void *value = __atomic_load_n(&node->value,__ATOMIC_ACQUIRE);
                    mark_obj(value, type16, mark_bit);
                    // if (vtype==102||vtype==104)
                    //     gc_list(value, vtype, mark_bit);
                    // else
                        worklist_push(value, vtype);
                }
                node = __atomic_load_n(&node->next,__ATOMIC_ACQUIRE);
            }
        }
    }
}


int marks = 0;

extern "C" void GC_array_append_str_barrier(uint16_t type, Scope_Struct *ctx, DT_array *vec, char *ptr, int str_size) {
    GC *gc = ctx->gc;
    int size = vec->size;
    int idx = vec->virtual_size;
    if (idx>=size)
        array_double_size(ctx, vec);
    
    void **slot = (void**)((char*)vec->data + idx*16);

    GC_Arena *arena = gc->arena;
    marks++;
    
    __atomic_store_n(slot, ptr, __ATOMIC_RELEASE);
    __atomic_store_n((int*)((char*)slot+8), str_size, __ATOMIC_RELEASE);
    __atomic_store_n(&vec->virtual_size, idx+1, __ATOMIC_RELEASE);

    bool is_marked = arena->get_is_marked(vec, gc->mark_bit);
    if(is_marked&&ptr!=nullptr)
        arena->mutator_push(ptr, type);
}

extern "C" void GC_array_append_barrier(uint16_t type, Scope_Struct *ctx, DT_array *vec, void *ptr) {
    GC *gc = ctx->gc;
    int size = vec->size;
    int idx = vec->virtual_size;
    if (idx>=size)
        array_double_size(ctx, vec);
    
    if (type<100) {
        std::cout << "APPEND PRIMARY " << type << "\n";
        std::exit(0);
    }

    void **data = (void**)vec->data;
    void **slot = &data[idx];

    GC_Arena *arena = gc->arena;
    marks++;
    
    __atomic_store_n(slot, ptr, __ATOMIC_RELEASE);
    __atomic_store_n(&vec->virtual_size, idx+1, __ATOMIC_RELEASE);
    bool is_marked = arena->get_is_marked(vec, gc->mark_bit);
    if(is_marked&&ptr!=nullptr)
        arena->mutator_push(ptr, type);
}


extern "C" void GC_write_barrier_str(uint16_t type, GC *gc, void *src, void **slot, char *ptr, int size) {
    marks++;
    GC_Arena *arena = gc->arena;

    // bool marking = __atomic_load_n(&gc->marking, __ATOMIC_ACQUIRE);
    // if (marking) {
        // void* old = __atomic_load_n(slot, __ATOMIC_ACQUIRE);
        // bool is_marked = arena->get_is_marked(src, gc->mark_bit);
        // if(!is_marked&&old!=nullptr)
        //     arena->mutator_push(old, type);
    // }
    __atomic_store_n(slot, ptr, __ATOMIC_RELEASE);
    __atomic_store_n((int*)((char*)slot+8), size, __ATOMIC_RELEASE);
    bool is_marked = arena->get_is_marked(src, gc->mark_bit);
    if(is_marked&&ptr!=nullptr)
        arena->mutator_push(ptr, type);
}
extern "C" void GC_write_barrier_obj(uint16_t type, GC *gc, void *src, void **slot, void *ptr) {
    marks++;
    GC_Arena *arena = gc->arena;
    
    // Yuasa
    // bool marking = __atomic_load_n(&gc->marking, __ATOMIC_ACQUIRE);
    // if (marking) {
        // void* old = __atomic_load_n(slot, __ATOMIC_ACQUIRE);
        // bool is_marked = arena->get_is_marked(src, gc->mark_bit);
        // if(!is_marked&&old!=nullptr)
        //     arena->mutator_push(old, type);
        // std::cout << "mark barrier " << old << ", set to " << " ptr " << "\n";
    // }
    __atomic_store_n(slot, ptr, __ATOMIC_RELEASE);
    // Dijkstra
    bool is_marked = arena->get_is_marked(src, gc->mark_bit);
    if(is_marked&&ptr!=nullptr)
        arena->mutator_push(ptr, type);
}

bool GC_Arena::mark_worklist_pointers(Scope_Struct *scope_struct, uint64_t mark_bit) {
    uint16_t type16;
    GC_Node node;

    std::unique_lock<std::mutex> lock(sweep_mtx, std::defer_lock);
    bool locked=false;
    while (scope_struct->alive) {
        if (!topw) {
            if (!locked) {
                lock.lock();
                locked = true;
            }
            topw = mutatorw.exchange(nullptr, std::memory_order_acquire);
            if(!topw)
                break;
        }

        node = topw->node;
        walkw();
        if(!mark_obj(node.ptr, type16, mark_bit))
            continue;
        TypeInfo *class_info = type_info[type16]; 
        if (class_info!=nullptr) {
            for (int ptr_i=0; ptr_i<class_info->pointers_count; ++ptr_i) {
                PtrInfo *ptr_info = &class_info->ptr_info[ptr_i];
                uint16_t offset = ptr_info->offset;
                uint16_t nested_type = ptr_info->type;
                
                if(!node.ptr)
                    continue;
                void** slot_ptr = reinterpret_cast<void**>(
                    static_cast<char*>(node.ptr) + offset
                );
                void* slot = __atomic_load_n(slot_ptr, __ATOMIC_ACQUIRE);

                if(slot)
                    worklist_push(slot, nested_type);
            }
        }
        gc_list(node.ptr, type16, mark_bit);
    }
    if (!locked)
        lock.lock();

    __atomic_store_n(&gc->marking, false, __ATOMIC_RELEASE);
    topw=nullptr;
    if(scope_struct->alive)
        gc->CleanUp_Unused(scope_struct->thread_id);
    lock.unlock();
    return true;
}

void GC_Arena::check_roots_worklist(Scope_Struct *scope_struct, uint64_t mark_bit) {
    uint16_t type16;

    int stack_size = __atomic_load_n(&scope_struct->stack_top, __ATOMIC_ACQUIRE); 
    // std::cout << "stack size: " << stack_size << "\n";
    for (int i=0; i<stack_size; ++i) {
        void *root_ptr = scope_struct->pointers_stack[i];
        // std::cout << "root " << root_ptr << "  --  ";
        if (!mark_obj(root_ptr, type16, mark_bit)) {
            // std::cout << "\n";
            continue;
        }
        // std::cout << "" << type16 << "\n";

        TypeInfo *class_info = type_info[type16]; 
        if (class_info!=nullptr) {
            for (int ptr_i=0; ptr_i<class_info->pointers_count; ++ptr_i) {
                PtrInfo *ptr_info = &class_info->ptr_info[ptr_i];
                int offset = ptr_info->offset;
                uint16_t nested_type = ptr_info->type;

                void** slot_ptr = reinterpret_cast<void**>(
                    static_cast<char*>(root_ptr) + offset
                );
                void* slot = __atomic_load_n(slot_ptr, __ATOMIC_ACQUIRE);
                if(slot!=nullptr)
                    worklist_push(slot, nested_type);
            }
        }
        gc_list(root_ptr, type16, mark_bit);
    }
}



// sweep
void GC::Sweep(Scope_Struct *scope_struct) {
    int tid = scope_struct->thread_id;
    __atomic_store_n(&marking, true, __ATOMIC_RELEASE);
    std::unique_lock<std::mutex> lock(arena->sweep_mtx, std::defer_lock);

    // std::cout << "sweep" << "\n";
    arena->gen += 2;

    lock.lock();
    arena->check_roots_worklist(scope_struct, mark_bit);
    lock.unlock();
    arena->mark_worklist_pointers(scope_struct, mark_bit);
    // std::cout << "sweep" << "\n";
    __atomic_store_n(&marking, false, __ATOMIC_RELEASE);
    allocations=0;
    size_occupied=0;
}



void GC::CleanUp_Unused(int tid) {
    for (int span_group=0; span_group<GC_obj_sizes; span_group++) {
        // GC_Span *span_ST = nullptr, *cur=arena->current_span[span_group];
        // for (const auto &span : arena->Spans[span_group]) {
        //     if (span==cur) continue;
        //     span->sweeped=false;
        //     span->next_span = span_ST;
        //     span_ST = span;
        // }
        // if (cur)
        //     cur->next_span = span_ST;

        
        GC_Span *span_ST = nullptr, *free_span_ST = nullptr, *last_free = nullptr;
        int span_id=-1;
        for (const auto &span : arena->Spans[span_group]) {
            span_id++;

            int obj_size = span->elem_size;
            int free_slots = 0; 
            
            span->Sweep(tid, mark_bit);

            // std::cout << free_slots << " | " << span->N << " -- " << obj_size << "|" << span_id << "\n";
            bool is_free = (span->free_idx<span->N||free_slots==span->N);
            if (is_free) {
                if(!last_free)
                    last_free = span;
                span->next_span = free_span_ST;
                free_span_ST = span;
            } else {
                span->next_span = span_ST;
                span_ST = span;
            }
            if (free_slots>=span->N) {
                // span->cur_free = (char*)span->span_address;
                span->free_idx=0;
            }
        }
        GC_Span *first_span = span_ST;
        if (last_free) {
            last_free->next_span = span_ST;
            first_span = free_span_ST;
        }
        arena->current_span[span_group] = first_span;
    }
    mark_bit ^= 1;
}




void GC::retire_arr(void *data, int size, int tid) {
    DT_array_retire *x = new DT_array_retire(data, size, tid);
    CAS_push(x, retired_arr);
}

void GC::retire_node(DT_map_node *data, int tid) {
    DT_node_retire *x = new DT_node_retire(data, tid);
    CAS_push(x, retired_nodes);
}


void GC::retire_clean() {
    // array vec
    DT_array_retire *vec_list =
        __atomic_exchange_n(&retired_arr,
                            nullptr,
                            __ATOMIC_ACQUIRE);
    while (vec_list) {
        cache_push(vec_list->data, vec_list->size, vec_list->tid);
        DT_array_retire *old = vec_list;
        vec_list = vec_list->next;
        delete old;
    }

    // map nodes
    DT_node_retire *nodes_list =
        __atomic_exchange_n(&retired_nodes,
                            nullptr,
                            __ATOMIC_ACQUIRE);
    while (nodes_list) {
        if(!check_is_in_bounds((char*)arena->arena, (char*)nodes_list->data->key))
            free(nodes_list->data->key);
        if(!check_is_in_bounds((char*)arena->arena, (char*)nodes_list->data->value))
            free(nodes_list->data->value);
        free(nodes_list->data);
        DT_node_retire *old = nodes_list;
        nodes_list = nodes_list->next;
        delete old;
    }
}

