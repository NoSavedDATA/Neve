#include <cstdint>
#include <stdio.h>
#include <string>

#include "../common/extension_functions.h"
#include "../compiler_frontend/logging_v.h"
#include "../pool/include.h"
#include "../pool/pool.h"
#include "array.h"

#include "data_tree.h"
#include "map.h"
#include "list.h"
#include "str_view.h"



DT_map_node::DT_map_node(int key_size, int value_size) {
    key = malloc(key_size);
    value = malloc(value_size);
}

DT_map::DT_map() {}

void DT_map::New(Scope_Struct *ctx, int size, int key_size, int value_size, std::string key_type, std::string value_type) {
    // std::unique_lock<std::mutex> lock(ctx->gc->arena->sweep_mtx);
    this->size = size;
    this->key_size = key_size;
    this->val_size = value_size;
    this->key_type = key_type;
    this->val_type = value_type;

    // size = 1200;
    capacity = ((size + 7) / 8) * 8;
    if (capacity<8)
        capacity=8;

    expand_at = capacity*4;

    nodes = (DT_map_node**)malloc(capacity*8); // 8 == size of one void *
    for (int i=0; i<capacity; ++i)
        nodes[i] = nullptr;
    // std::cout << "create map: " << key_size << "/" << value_size  << "/" << key_type << ".\n";
}

extern "C" DT_map *map_Create(Scope_Struct *scope_struct, Data_Tree dt) {
    if (dt.Nested_Data.size()<2)
        LogErrorC(scope_struct->code_line, "map requires key and value info");

    std::string key_type = dt.Nested_Data[0].Type;
    int key_size;
    if(data_name_to_size.count(key_type)>0)
        key_size = data_name_to_size[key_type];
    else
        key_size = 8;

    std::string value_type = dt.Nested_Data[1].Type;
    int value_size;
    if(data_name_to_size.count(value_type)>0)
        value_size = data_name_to_size[value_type];
    else
        value_size = 8;

    DT_map *map = newT<DT_map>(scope_struct, "map");
    map->New(scope_struct, 0, key_size, value_size, key_type, value_type); 

    
    return map;
}


std::unordered_map<DT_map_node*,int> cleaned_nodes;

void map_Clean_Up(void *ptr, int tid) {
    DT_map *map = static_cast<DT_map*>(ptr);

    int capacity = __atomic_load_n(&map->capacity, __ATOMIC_ACQUIRE);
    DT_map_node **nodes = __atomic_load_n(&map->nodes,__ATOMIC_ACQUIRE);


    bool key_primary = data_name_to_type()[map->key_type] < 100;
    bool val_primary = data_name_to_type()[map->val_type] < 100;

    for (int i=0; i<capacity; ++i) {
        DT_map_node *node = nodes[i];
        while (node!=nullptr) {
            DT_map_node *next = node->next;
            if(key_primary)
                free(node->key);
            if(val_primary) 
                free(node->value);
            free(node);
            node = next;
        }
    }
    free(nodes);
}


DT_node_retire::DT_node_retire(DT_map_node *data, int tid)
            : data(data), tid(tid) {}

extern "C" int map_node_reclaim(Scope_Struct *ctx, DT_map *map, DT_map_node *node) {
    // std::cout << "reclaim map " << map << ", node " << node << "\n";
    GC *gc = ctx->gc;
    gc->retire_node(node, ctx->thread_id);
    
    return 0;
}

extern "C" int map_size(Scope_Struct *scope_struct, DT_map *map) {
    return map->size;
}
// extern "C" int map_size(Scope_Struct *scope_struct, DT_map *map) {
//     int size=0;
//     for (int i=0; i<map->capacity; ++i) {
//         DT_map_node *node = map->nodes[i];
//         while (node!=nullptr) {
//             size++;
//             node = node->next;
//         }
//     }
//     std::cout << "size " << size << " | " << map->size << "\n";
//     return size;
// }

void DT_map::Insert(int hash_pos, DT_map_node *node, DT_map_node **nodes) {
    DT_map_node *cur_node = nodes[hash_pos];

    if (cur_node==nullptr)
        __atomic_store_n(&nodes[hash_pos], node, __ATOMIC_RELEASE);
    else {
        while(cur_node->next!=nullptr)
            cur_node = cur_node->next;
        __atomic_store_n(&cur_node->next, node, __ATOMIC_RELEASE);
    }
}

extern "C" void map_expand(Scope_Struct *scope_struct, DT_map *map) {
    int capacity = map->capacity*32;

    DT_map_node **nodes = (DT_map_node**)malloc(capacity*8); // 8 == size of one void *
    for (int i=0; i<capacity; i++)
        nodes[i] = nullptr;
    
    for (int i=0; i<map->capacity; ++i) {
        DT_map_node *node = map->nodes[i];
        while (node!=nullptr) {
            DT_map_node *next = node->next;
            unsigned int hashed;
            if (map->key_type=="str") {
                char *key = static_cast<char*>(node->key);
                hashed = str_hash(key);
            } else if (map->key_type=="int") {
                int *key = static_cast<int*>(node->key);
                hashed = *key;
                unsigned int bucket_pos = hashed % capacity;
            } else if (map->key_type=="i64") {
                int64_t *key = static_cast<int64_t*>(node->key);
                hashed = *key;
                unsigned int bucket_pos = hashed % capacity;
            } else if (map->key_type=="float") {
                float *key = static_cast<float*>(node->key);
                hashed = float_hash(*key);
            } else if (map->key_type=="array") {
                DT_array *key = static_cast<DT_array*>(node->key);
                if (key->type==2)
                    hashed = hash_array_int(scope_struct, key);
                else
                    LogErrorC(-1, "map expand not implemented for array of type " + map->key_type);

            } else {
                LogErrorC(-1, "map expand not implemented for " + map->key_type);
                std::exit(0);
            }
            
            unsigned int bucket_pos = hashed % capacity;
            
            node->next = nullptr;
            map->Insert(bucket_pos, node, nodes);
            
            node = next;
        }
    }

    free(map->nodes);
    __atomic_store_n(&map->nodes, nodes, __ATOMIC_RELEASE);
    __atomic_store_n(&map->capacity, capacity, __ATOMIC_RELEASE);
    __atomic_store_n(&map->expand_at, capacity*32, __ATOMIC_RELEASE);
}


extern "C" bool map_has_str(Scope_Struct *ctx, DT_map *map, DT_str query) {
    std::unique_lock<std::mutex> lock(ctx->gc->arena->sweep_mtx);
    int hash_pos = str_hash(query.str, query.size) % map->capacity;
    DT_map_node *cur_node = map->nodes[hash_pos];
    while(cur_node!=nullptr) {
        char *key = static_cast<char*>(cur_node->key);
        if(query.size == strlen(key) &&
           std::memcmp(query.str, key, query.size) == 0)
            return true;
        cur_node = cur_node->next;
    }
    return false;
}
extern "C" bool map_has_int(Scope_Struct *scope_struct, DT_map *map, int query) {
    int hash_pos = query % map->capacity;
    DT_map_node *cur_node = map->nodes[hash_pos];
    while(cur_node!=nullptr) {
        int *key = static_cast<int*>(cur_node->key);
        if (query == *key)
            return true;
        cur_node = cur_node->next;
    }
    return false;
}
extern "C" bool map_has_i64(Scope_Struct *scope_struct, DT_map *map, int64_t query) {
    int hash_pos = query % map->capacity;
    DT_map_node *cur_node = map->nodes[hash_pos];
    while(cur_node!=nullptr) {
        int64_t *key = static_cast<int64_t*>(cur_node->key);
        if (query == *key)
            return true;
        cur_node = cur_node->next;
    }
    return false;
}
extern "C" bool map_has_float(Scope_Struct *scope_struct, DT_map *map, float query) {
    int hash_pos = float_hash(query) % map->capacity;
    DT_map_node *cur_node = map->nodes[hash_pos];
    while(cur_node!=nullptr) {
        float *key = static_cast<float*>(cur_node->key);
        if (query == *key)
            return true;
        cur_node = cur_node->next;
    }
    return false;
}
extern "C" bool map_has_char(Scope_Struct *scope_struct, DT_map *map, int query) {
    int hash_pos = query % map->capacity;
    DT_map_node *cur_node = map->nodes[hash_pos];
    while(cur_node!=nullptr) {
        int *key = static_cast<int*>(cur_node->key);
        if (query == *key)
            return true;
        cur_node = cur_node->next;
    }
    return false;
}


extern "C" void print_str(char *str) {
    std::cout << "print_str: " << str << ".\n";
}

extern "C" int map_print(Scope_Struct *scope_struct, DT_map *map) {
    map->print();
    return 0;
}


extern "C" int map_node_set_bucket(Scope_Struct *scope_struct, DT_map *map,
                        DT_map_node *new_node, int hash_pos) {
    __atomic_store_n(&map->nodes[hash_pos], new_node, __ATOMIC_RELEASE);
    int size = __atomic_load_n(&map->size, __ATOMIC_ACQUIRE); 
    __atomic_store_n(&map->size, size+1, __ATOMIC_RELEASE);
    return 0;
}
extern "C" int map_node_set_next(Scope_Struct *scope_struct, DT_map *map, DT_map_node *new_node,
                        DT_map_node *bucket_last) {
    __atomic_store_n(&bucket_last->next, new_node, __ATOMIC_RELEASE);
    int size = __atomic_load_n(&map->size, __ATOMIC_ACQUIRE); 
    __atomic_store_n(&map->size, size+1, __ATOMIC_RELEASE);
    return 0;
}
extern "C" int map_node_overwrite_bucket(Scope_Struct *scope_struct, DT_map_node *new_node,
                        DT_map *map, int hash_pos) {
    DT_map_node *old_node = __atomic_load_n(&map->nodes[hash_pos], __ATOMIC_ACQUIRE);
    DT_map_node *old_node_next = __atomic_load_n(&old_node->next, __ATOMIC_ACQUIRE);
    __atomic_store_n(&new_node->next, old_node_next, __ATOMIC_RELEASE);
    __atomic_store_n(&map->nodes[hash_pos], new_node, __ATOMIC_RELEASE);

    return 0;
}
extern "C" int map_node_overwrite(Scope_Struct *scope_struct, DT_map *map, DT_map_node *new_node,
                        DT_map_node *prev, DT_map_node *replaced) {
    DT_map_node *replaced_next = __atomic_load_n(&replaced->next, __ATOMIC_ACQUIRE);
    __atomic_store_n(&new_node->next, replaced_next, __ATOMIC_RELEASE);
    __atomic_store_n(&prev->next, new_node, __ATOMIC_RELEASE);
    return 0;
}


extern "C" DT_array *map_keys_str(Scope_Struct *scope_struct, DT_map *map) {
    DT_array *array = newT<DT_array>(scope_struct, "array");
    array->New(scope_struct, map->size, map->key_size, scope_struct->thread_id, data_name_to_type()[map->key_type]);

    DT_str *vec = static_cast<DT_str*>(array->data);
    
    int idx=0;
    for (int i=0; i<map->capacity; ++i) {
        DT_map_node *node = map->nodes[i];
        while (node!=nullptr) {
            char *key = static_cast<char*>(node->key);
            int size = strlen(key);
            vec[idx++] = DT_str(key, size);
            node = node->next;
        }
    }
    array->virtual_size = idx;
    return array;
}

extern "C" DT_array *map_keys_array(Scope_Struct *scope_struct, DT_map *map) {
    DT_array *array = newT<DT_array>(scope_struct, "array");
    array->New(scope_struct, map->size, map->key_size, scope_struct->thread_id, data_name_to_type()[map->key_type]);

    DT_array **vec = static_cast<DT_array**>(array->data);
    
    int idx=0;
    for (int i=0; i<map->capacity; ++i) {
        DT_map_node *node = map->nodes[i];
        while (node!=nullptr) {
            DT_array *key = static_cast<DT_array*>(node->key);
            vec[idx++] = key;
            node = node->next;
        }
    }
    array->virtual_size = idx;
    return array;
}

extern "C" DT_array *map_keys_i64(Scope_Struct *scope_struct, DT_map *map) {
    DT_array *array = newT<DT_array>(scope_struct, "array");
    array->New(scope_struct, map->size, map->key_size, scope_struct->thread_id, data_name_to_type()[map->key_type]);

    int64_t *vec = static_cast<int64_t*>(array->data);
    
    int idx=0;
    for (int i=0; i<map->capacity; ++i) {
        DT_map_node *node = map->nodes[i];
        while (node!=nullptr) {
            int64_t *key = static_cast<int64_t*>(node->key);
            vec[idx++] = *key;
            node = node->next;
        }
    }
    array->virtual_size = idx;
    return array;
}

extern "C" DT_array *map_keys(Scope_Struct *scope_struct, DT_map *map) {
    DT_array *array = newT<DT_array>(scope_struct, "array");
    array->New(scope_struct, map->size, map->key_size, scope_struct->thread_id, data_name_to_type()[map->key_type]);

    
    int idx=0;
    for (int i=0; i<map->capacity; ++i) {
        DT_map_node *node = map->nodes[i];
        while (node!=nullptr) {
            if (map->key_type=="int") {
                int *key = static_cast<int*>(node->key);
                int *vec = static_cast<int*>(array->data);
                vec[idx] = *key;
            } else if (map->key_type=="float") {
                float *key = static_cast<float*>(node->key);
                float *vec = static_cast<float*>(array->data);
                vec[idx] = *key;
            } else if (map->key_type=="array") {
                DT_array *key = static_cast<DT_array*>(node->key);
                DT_array **vec = static_cast<DT_array**>(array->data);
                vec[idx] = key;
            }             
            
            idx++;
            node = node->next;
        }
    }
    array->virtual_size = idx;
    return array;
}

extern "C" DT_array *map_values(Scope_Struct *scope_struct, DT_map *map) {
    DT_array *array = newT<DT_array>(scope_struct, "array");
    array->New(scope_struct, map->size, map->val_size, scope_struct->thread_id, data_name_to_type()[map->val_type]);
    
    int idx=0;
    for (int i=0; i<map->capacity; ++i) {
        DT_map_node *node = map->nodes[i];
        while (node!=nullptr) {
            if (map->val_type=="str") {
                char *value = static_cast<char*>(node->value);
                DT_str *vec = static_cast<DT_str*>(array->data);
                int size = strlen(value);
                vec[idx] = DT_str(value, size);
            } else if (map->val_type=="int") {
                int *vec = static_cast<int*>(array->data);
                int *value = static_cast<int*>(node->value);
                vec[idx] = *value;
            } else if (map->key_type=="i64") {
                int64_t *value = static_cast<int64_t*>(node->value);
                int64_t *vec = static_cast<int64_t*>(array->data);
                vec[idx] = *value;
            } else if (map->val_type=="float") {
                float *value = static_cast<float*>(node->value);
                float *vec = static_cast<float*>(array->data);
                vec[idx] = *value;
            } else if (map->key_type=="array") {
                DT_array *value = static_cast<DT_array*>(node->value);
                DT_array **vec = static_cast<DT_array**>(array->data);
                vec[idx] = value;
            } else {
                LogErrorC(-1, "unsupported keys of " + map->key_type);
                std::exit(0);
            }
            
            idx++;
            node = node->next;
        }
    }
    array->virtual_size = idx;
    return array;
}

extern "C" DT_array *map_values_int(Scope_Struct *scope_struct, DT_map *map) {
    DT_array *array = newT<DT_array>(scope_struct, "array");
    array->New(scope_struct, map->size, map->val_size, scope_struct->thread_id, data_name_to_type()[map->val_type]);
    int *vec = static_cast<int*>(array->data);
    
    int idx=0;
    for (int i=0; i<map->capacity; ++i) {
        DT_map_node *node = map->nodes[i];
        while (node!=nullptr) {
            int *value = static_cast<int*>(node->value);
            vec[idx++] = *value;
            node = node->next;
        }
    }
    array->virtual_size = idx;
    return array;
}


extern "C" void map_bad_key_str(Scope_Struct *scope_struct, char *key) {
    std::string key_str = key;
    LogErrorC(scope_struct->code_line, "Map does not contain key: " + key_str);
    std::exit(0);
}

extern "C" void map_bad_key_int(Scope_Struct *scope_struct, int key) {
    LogErrorC(scope_struct->code_line, "Map does not contain key: " + std::to_string(key));
    std::exit(0);
}
extern "C" void map_bad_key_i64(Scope_Struct *scope_struct, int64_t key) {
    LogErrorC(scope_struct->code_line, "Map does not contain key: " + std::to_string(key));
    std::exit(0);
}
extern "C" void map_bad_key_array(Scope_Struct *scope_struct, DT_array *key) {
    LogErrorC(scope_struct->code_line, "Map does not contain array: ");
    std::exit(0);
}

extern "C" void map_bad_key_float(Scope_Struct *scope_struct, float key) {
    LogErrorC(scope_struct->code_line, "Map does not contain key: " + std::to_string(key));
    std::exit(0);
}


extern "C" int map_clear(Scope_Struct *ctx, DT_map *map) {
    GC *gc = ctx->gc;

    int capacity = __atomic_load_n(&map->capacity, __ATOMIC_ACQUIRE);
    for (int i=0; i<capacity; ++i) {
        DT_map_node *node = __atomic_exchange_n(&map->nodes[i], nullptr, __ATOMIC_ACQ_REL);
        while (node!=nullptr) {
            DT_map_node *next = __atomic_load_n(&node->next, __ATOMIC_ACQUIRE);
            gc->retire_node(node, ctx->thread_id);
            node = next;
        }
    }
    __atomic_store_n(&map->size, 0, __ATOMIC_RELEASE);
    return 0;
}


void DT_map::print() {
    std::cout << "\n";
    for (int i=0; i<capacity; ++i) {
        DT_map_node *node = nodes[i];
        while (node!=nullptr) {
            if (key_type=="str") {
                char *key = static_cast<char*>(node->key);
                std::cout << "bucket[" << i << "]: " << key << "\n";
            } else if (key_type=="int") {
                int *key = static_cast<int*>(node->key);
                std::cout << "bucket[" << i << "]: " << *key << "\n";
            } else if (key_type=="i64") {
                int64_t *key = static_cast<int64_t*>(node->key);
                std::cout << "bucket[" << i << "]: " << *key << "\n";
            } else if (key_type=="float") {
                float *key = static_cast<float*>(node->key);
                std::cout << "bucket[" << i << "]: " << *key << "\n";
            }

            if (val_type=="int") {
                int *value = static_cast<int*>(node->value);
                std::cout << "val: " << *value << "\n\n";
            }
            else
                std::cout << "val: " << node->value << "\n\n";
            node = node->next;
        }
    }
}



void DT_map::insert(Scope_Struct *scope_struct, std::string key, void *value) {
    std::cout << "OUTDATED INSERT" << "\n";
    DT_map_node *new_node = (DT_map_node *)malloc(sizeof(DT_map_node));

    char *new_key = allocate<char>(scope_struct, key.size()+1, "str");
    memcpy(new_key, key.c_str(), key.size()+1);

    new_node->key = new_key;
    new_node->value = value;

    int hash_slot = str_hash(key.c_str()) % capacity;

    DT_map_node *node = nodes[hash_slot];

    if (node==nullptr) {
        nodes[hash_slot] = new_node;
        return;
    }
    
    if(key==(char*)node->key) {
        nodes[hash_slot] = new_node;
        return;
    }

    DT_map_node *prev;
    while(node!=nullptr) {
        if(key==(char*)node->key)
            break;
        prev = node;
        node = node->next;
    }
    prev->next = new_node;
}

void DT_map::insert(Scope_Struct *scope_struct, int key, void *value) {
    std::cout << "OUTDATED INSERT" << "\n";
    DT_map_node *new_node = (DT_map_node *)malloc(sizeof(DT_map_node));

    int *new_key = (int*)malloc(sizeof(int));
    *new_key = key;

    new_node->key = (void*)new_key;
    new_node->value = value;

    int hash_slot = key % capacity;

    DT_map_node *node = nodes[hash_slot];

    if (node==nullptr) {
        nodes[hash_slot] = new_node;
        return;
    }

    if(key==*(int*)node->key) {
        nodes[hash_slot] = new_node;
        return;
    }

    DT_map_node *prev;
    while(node!=nullptr) {
        if(key==*(int*)node->key)
            break;
        prev = node;
        node = node->next;
    }
    prev->next = new_node;
}

void DT_map::insert(Scope_Struct *scope_struct, float key, void *value) {
    std::cout << "OUTDATED INSERT" << "\n";
    DT_map_node *new_node = (DT_map_node *)malloc(sizeof(DT_map_node));

    float *new_key = (float*)malloc(sizeof(float));
    *new_key = key;

    new_node->key = (void*)new_key;
    new_node->value = value;

    int hash_slot = float_hash(key) % capacity;

    DT_map_node *node = nodes[hash_slot];

    if (node==nullptr) {
        nodes[hash_slot] = new_node;
        return;
    }

    if(key==*(float*)node->key) {
        nodes[hash_slot] = new_node;
        return;
    }

    DT_map_node *prev;
    while(node!=nullptr) {
        if(key==*(float*)node->key)
            break;
        prev = node;
        node = node->next;
    }
    prev->next = new_node;
    
}




void DT_map::insert(Scope_Struct *scope_struct, std::string key, int value) {
    std::cout << "OUTDATED INSERT" << "\n";
    DT_map_node *new_node = (DT_map_node *)malloc(sizeof(DT_map_node));

    char *new_key = allocate<char>(scope_struct, key.size()+1, "str");
    memcpy(new_key, key.c_str(), key.size()+1);

    int *val = (int*)malloc(sizeof(int));
    *val = value;

    new_node->key = new_key;
    new_node->value = val;

    int hash_slot = str_hash(key.c_str()) % capacity;

    DT_map_node *node = nodes[hash_slot];

    if (node==nullptr) {
        nodes[hash_slot] = new_node;
        return;
    }
    
    if(key==(char*)node->key) {
        nodes[hash_slot] = new_node;
        return;
    }

    DT_map_node *prev;
    while(node!=nullptr) {
        if(key==(char*)node->key)
            break;
        prev = node;
        node = node->next;
    }
    prev->next = new_node;
}

void DT_map::insert(Scope_Struct *scope_struct, int key, int value) {
    std::cout << "OUTDATED INSERT" << "\n";
    DT_map_node *new_node = (DT_map_node *)malloc(sizeof(DT_map_node));

    int *new_key = (int*)malloc(sizeof(int));
    *new_key = key;

    int *val = (int*)malloc(sizeof(int));
    *val = value;

    new_node->key = new_key;
    new_node->value = val;

    int hash_slot = key % capacity;

    DT_map_node *node = nodes[hash_slot];

    if (node==nullptr) {
        nodes[hash_slot] = new_node;
        return;
    }

    if(key==*(int*)node->key) {
        nodes[hash_slot] = new_node;
        return;
    }

    DT_map_node *prev;
    while(node!=nullptr) {
        if(key==*(int*)node->key)
            break;
        prev = node;
        node = node->next;
    }
    prev->next = new_node;
}

void DT_map::insert(Scope_Struct *scope_struct, float key, int value) {
    std::cout << "OUTDATED INSERT" << "\n";
    DT_map_node *new_node = (DT_map_node *)malloc(sizeof(DT_map_node));

    float *new_key = (float*)malloc(sizeof(float));
    *new_key = key;

    int *val = (int*)malloc(sizeof(int));
    *val = value;

    new_node->key = new_key;
    new_node->value = val;


    int hash_slot = float_hash(key) % capacity;

    DT_map_node *node = nodes[hash_slot];

    if (node==nullptr) {
        nodes[hash_slot] = new_node;
        return;
    }

    if(key==*(float*)node->key) {
        nodes[hash_slot] = new_node;
        return;
    }

    DT_map_node *prev;
    while(node!=nullptr) {
        if(key==*(float*)node->key)
            break;
        prev = node;
        node = node->next;
    }
    prev->next = new_node;
    
}


void DT_map::insert(Scope_Struct *scope_struct, std::string key, float value) {
    std::cout << "OUTDATED INSERT" << "\n";
    DT_map_node *new_node = (DT_map_node *)malloc(sizeof(DT_map_node));

    char *new_key = allocate<char>(scope_struct, key.size()+1, "str");
    memcpy(new_key, key.c_str(), key.size()+1);

    float *val = (float*)malloc(sizeof(float));
    *val = value;

    new_node->key = new_key;
    new_node->value = val;

    int hash_slot = str_hash(key.c_str()) % capacity;

    DT_map_node *node = nodes[hash_slot];

    if (node==nullptr) {
        nodes[hash_slot] = new_node;
        return;
    }
    
    if(key==(char*)node->key) {
        nodes[hash_slot] = new_node;
        return;
    }

    DT_map_node *prev;
    while(node!=nullptr) {
        if(key==(char*)node->key)
            break;
        prev = node;
        node = node->next;
    }
    prev->next = new_node;
}

void DT_map::insert(Scope_Struct *scope_struct, int key, float value) {
    std::cout << "OUTDATED INSERT" << "\n";
    DT_map_node *new_node = (DT_map_node *)malloc(sizeof(DT_map_node));

    int *new_key = (int*)malloc(sizeof(int));
    *new_key = key;

    float *val = (float*)malloc(sizeof(float));
    *val = value;

    new_node->key = new_key;
    new_node->value = val;

    int hash_slot = key % capacity;

    DT_map_node *node = nodes[hash_slot];

    if (node==nullptr) {
        nodes[hash_slot] = new_node;
        return;
    }

    if(key==*(int*)node->key) {
        nodes[hash_slot] = new_node;
        return;
    }

    DT_map_node *prev;
    while(node!=nullptr) {
        if(key==*(int*)node->key)
            break;
        prev = node;
        node = node->next;
    }
    prev->next = new_node;
}

void DT_map::insert(Scope_Struct *scope_struct, float key, float value) {
    std::cout << "OUTDATED INSERT" << "\n";
    DT_map_node *new_node = (DT_map_node *)malloc(sizeof(DT_map_node));

    float *new_key = (float*)malloc(sizeof(float));
    *new_key = key;

    float *val = (float*)malloc(sizeof(float));
    *val = value;

    new_node->key = new_key;
    new_node->value = val;


    int hash_slot = float_hash(key) % capacity;

    DT_map_node *node = nodes[hash_slot];

    if (node==nullptr) {
        nodes[hash_slot] = new_node;
        return;
    }

    if(key==*(float*)node->key) {
        nodes[hash_slot] = new_node;
        return;
    }

    DT_map_node *prev;
    while(node!=nullptr) {
        if(key==*(float*)node->key)
            break;
        prev = node;
        node = node->next;
    }
    prev->next = new_node;
    
}







void DT_map::insert(Scope_Struct *scope_struct, std::string key, bool value) {
    std::cout << "OUTDATED INSERT" << "\n";
    DT_map_node *new_node = (DT_map_node *)malloc(sizeof(DT_map_node));

    char *new_key = allocate<char>(scope_struct, key.size()+1, "str");
    memcpy(new_key, key.c_str(), key.size()+1);

    bool *val = (bool*)malloc(sizeof(bool));
    *val = value;

    new_node->key = new_key;
    new_node->value = val;

    int hash_slot = str_hash(key.c_str()) % capacity;

    DT_map_node *node = nodes[hash_slot];

    if (node==nullptr) {
        nodes[hash_slot] = new_node;
        return;
    }
    
    if(key==(char*)node->key) {
        nodes[hash_slot] = new_node;
        return;
    }

    DT_map_node *prev;
    while(node!=nullptr) {
        if(key==(char*)node->key)
            break;
        prev = node;
        node = node->next;
    }
    prev->next = new_node;
}

void DT_map::insert(Scope_Struct *scope_struct, int key, bool value) {
    std::cout << "OUTDATED INSERT" << "\n";
    DT_map_node *new_node = (DT_map_node *)malloc(sizeof(DT_map_node));

    int *new_key = (int*)malloc(sizeof(int));
    *new_key = key;

    bool *val = (bool*)malloc(sizeof(bool));
    *val = value;

    new_node->key = new_key;
    new_node->value = val;

    int hash_slot = key % capacity;

    DT_map_node *node = nodes[hash_slot];

    if (node==nullptr) {
        nodes[hash_slot] = new_node;
        return;
    }

    if(key==*(int*)node->key) {
        nodes[hash_slot] = new_node;
        return;
    }

    DT_map_node *prev;
    while(node!=nullptr) {
        if(key==*(int*)node->key)
            break;
        prev = node;
        node = node->next;
    }
    prev->next = new_node;
}

void DT_map::insert(Scope_Struct *scope_struct, float key, bool value) {
    std::cout << "OUTDATED INSERT" << "\n";
    DT_map_node *new_node = (DT_map_node *)malloc(sizeof(DT_map_node));

    bool *new_key = (bool*)malloc(sizeof(bool));
    *new_key = key;

    float *val = (float*)malloc(sizeof(float));
    *val = value;

    new_node->key = new_key;
    new_node->value = val;


    int hash_slot = float_hash(key) % capacity;

    DT_map_node *node = nodes[hash_slot];

    if (node==nullptr) {
        nodes[hash_slot] = new_node;
        return;
    }

    if(key==*(float*)node->key) {
        nodes[hash_slot] = new_node;
        return;
    }

    DT_map_node *prev;
    while(node!=nullptr) {
        if(key==*(float*)node->key)
            break;
        prev = node;
        node = node->next;
    }
    prev->next = new_node;
    
}







