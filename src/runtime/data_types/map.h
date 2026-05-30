#pragma once

#include "../compiler_frontend/logging_v.h"

#include <cstring>
#include <mutex>
#include <string>

inline uint32_t str_hash(const char* str, std::size_t len) {
    uint32_t hash = 2166136261u;

    for (std::size_t i = 0; i < len; ++i) {
        hash ^= static_cast<uint8_t>(str[i]);
        hash *= 16777619u;
    }

    return hash;
}
inline unsigned int str_hash(const char *str) {
    unsigned int hash = 2166136261u; // FNV-1a offset basis

    while (*str) {
        hash ^= static_cast<unsigned char>(*str);
        hash *= 16777619u;
        ++str;
    }
    return hash;
}
inline uint32_t float_hash(float x) {
    uint32_t bits;
    static_assert(sizeof(float) == sizeof(uint32_t));
    std::memcpy(&bits, &x, sizeof(bits));

    // Mix bits (Murmur3 finalizer-style)
    bits ^= bits >> 16;
    bits *= 0x85ebca6bU;
    bits ^= bits >> 13;
    bits *= 0xc2b2ae35U;
    bits ^= bits >> 16;

    return bits;
}


struct DT_map_node {
    void *key, *value;
    DT_map_node *next=nullptr;

    DT_map_node(int, int);
};
struct DT_node_retire {
    DT_node_retire *next;
    DT_map_node *data;
    int tid;
    DT_node_retire(DT_map_node *data, int);
};

struct DT_map {
    int size;
    int capacity;
    int expand_at;
    int key_size;    // sizeof(key)
    int val_size;    // sizeof(value)
    DT_map_node **nodes;
    std::string key_type;    // sizeof(key)
    std::string val_type;    // sizeof(value)
    std::mutex snapshot_mtx;

    DT_map();
    void New(Scope_Struct *, int, int, int, std::string, std::string);
    void Insert(int hash_pos, DT_map_node *node, DT_map_node **nodes);

    void print();

    // insert void*
    void insert(Scope_Struct *, std::string, void *);
    void insert(Scope_Struct *, int, void *);
    void insert(Scope_Struct *, float, void *);

    // insert int
    void insert(Scope_Struct *, std::string, int);
    void insert(Scope_Struct *, int, int);
    void insert(Scope_Struct *, float, int);

    // insert float
    void insert(Scope_Struct *, std::string, float);
    void insert(Scope_Struct *, int, float);
    void insert(Scope_Struct *, float, float);

    // insert bool
    void insert(Scope_Struct *, std::string, bool);
    void insert(Scope_Struct *, int, bool);
    void insert(Scope_Struct *, float, bool);

    template<typename T>
    T get(std::string key) {
        int hash_slot = str_hash(key.c_str()) % capacity;

        DT_map_node *node = nodes[hash_slot];

        if (node==nullptr) {
            LogErrorC(-1, "Key " + key + " not found.");
            std::exit(0);
        }

        if(key==(char*)node->key)
            return static_cast<T>(node->value);
        

        node = node->next;
        while(node!=nullptr) {
            if(key==(char*)node->key)
                return static_cast<T>(node->value);
            node = node->next;
        }
        
        LogErrorC(-1, "Key " + key + " not found.");
        std::exit(0);
    }

    template<typename T>
    T get(int key) {
        int hash_slot = key % capacity;

        DT_map_node *node = nodes[hash_slot];

        if (node==nullptr) {
            LogErrorC(-1, "Key " + std::to_string(key) + " not found.");
            std::exit(0);
        }

        if(key==*(int*)node->key)
            return static_cast<T>(node->value);
        

        node = node->next;
        while(node!=nullptr) {
            if(key==*(int*)node->key)
                return static_cast<T>(node->value);
            node = node->next;
        }
        
        LogErrorC(-1, "Key " + std::to_string(key) + " not found.");
        std::exit(0);
    }

    template<typename T>
    T get(float key) {
        int hash_slot = float_hash(key) % capacity;

        DT_map_node *node = nodes[hash_slot];

        if (node==nullptr) {
            LogErrorC(-1, "Key " + std::to_string(key) + " not found.");
            std::exit(0);
        }

        if(key==*(float*)node->key)
            return static_cast<T>(node->value);
        

        node = node->next;
        while(node!=nullptr) {
            if(key==*(float*)node->key)
                return static_cast<T>(node->value);
            node = node->next;
        }
        
        LogErrorC(-1, "Key " + std::to_string(key) + " not found.");
        std::exit(0);
    }










    template<>
    int get(std::string key) {
        int hash_slot = str_hash(key.c_str()) % capacity;

        DT_map_node *node = nodes[hash_slot];

        if (node==nullptr) {
            LogErrorC(-1, "Key " + key + " not found.");
            std::exit(0);
        }

        if(key==(char*)node->key)
            return *(int*)node->value;
        

        node = node->next;
        while(node!=nullptr) {
            if(key==(char*)node->key)
                return *(int*)node->value;
            node = node->next;
        }
        
        LogErrorC(-1, "Key " + key + " not found.");
        std::exit(0);
    }

    template<>
    int get(int key) {
        int hash_slot = key % capacity;

        DT_map_node *node = nodes[hash_slot];

        if (node==nullptr) {
            LogErrorC(-1, "Key " + std::to_string(key) + " not found.");
            std::exit(0);
        }

        if(key==*(int*)node->key)
            return *(int*)node->value;
        

        node = node->next;
        while(node!=nullptr) {
            if(key==*(int*)node->key)
                return *(int*)node->value;
            node = node->next;
        }
        
        LogErrorC(-1, "Key " + std::to_string(key) + " not found.");
        std::exit(0);
    }

    template<>
    int get(float key) {
        int hash_slot = float_hash(key) % capacity;

        DT_map_node *node = nodes[hash_slot];

        if (node==nullptr) {
            LogErrorC(-1, "Key " + std::to_string(key) + " not found.");
            std::exit(0);
        }

        if(key==*(float*)node->key)
            return *(int*)node->value;
        

        node = node->next;
        while(node!=nullptr) {
            if(key==*(float*)node->key)
                return *(int*)node->value;
            node = node->next;
        }
        
        LogErrorC(-1, "Key " + std::to_string(key) + " not found.");
        std::exit(0);
    }













    template<>
    float get(std::string key) {
        int hash_slot = str_hash(key.c_str()) % capacity;

        DT_map_node *node = nodes[hash_slot];

        if (node==nullptr) {
            LogErrorC(-1, "Key " + key + " not found.");
            std::exit(0);
        }

        if(key==(char*)node->key)
            return *(float*)node->value;
        

        node = node->next;
        while(node!=nullptr) {
            if(key==(char*)node->key)
                return *(float*)node->value;
            node = node->next;
        }
        
        LogErrorC(-1, "Key " + key + " not found.");
        std::exit(0);
    }

    template<>
    float get(int key) {
        int hash_slot = key % capacity;

        DT_map_node *node = nodes[hash_slot];

        if (node==nullptr) {
            LogErrorC(-1, "Key " + std::to_string(key) + " not found.");
            std::exit(0);
        }

        if(key==*(int*)node->key)
            return *(float*)node->value;
        

        node = node->next;
        while(node!=nullptr) {
            if(key==*(int*)node->key)
                return *(float*)node->value;
            node = node->next;
        }
        
        LogErrorC(-1, "Key " + std::to_string(key) + " not found.");
        std::exit(0);
    }

    template<>
    float get(float key) {
        int hash_slot = float_hash(key) % capacity;

        DT_map_node *node = nodes[hash_slot];

        if (node==nullptr) {
            LogErrorC(-1, "Key " + std::to_string(key) + " not found.");
            std::exit(0);
        }

        if(key==*(float*)node->key)
            return *(float*)node->value;
        

        node = node->next;
        while(node!=nullptr) {
            if(key==*(float*)node->key)
                return *(float*)node->value;
            node = node->next;
        }
        
        LogErrorC(-1, "Key " + std::to_string(key) + " not found.");
        std::exit(0);
    }




    template<>
    bool get(std::string key) {
        int hash_slot = str_hash(key.c_str()) % capacity;

        DT_map_node *node = nodes[hash_slot];

        if (node==nullptr) {
            LogErrorC(-1, "Key " + key + " not found.");
            std::exit(0);
        }

        if(key==(char*)node->key)
            return *(bool*)node->value;
        

        node = node->next;
        while(node!=nullptr) {
            if(key==(char*)node->key)
                return *(bool*)node->value;
            node = node->next;
        }
        
        LogErrorC(-1, "Key " + key + " not found.");
        std::exit(0);
    }

    template<>
    bool get(int key) {
        int hash_slot = key % capacity;

        DT_map_node *node = nodes[hash_slot];

        if (node==nullptr) {
            LogErrorC(-1, "Key " + std::to_string(key) + " not found.");
            std::exit(0);
        }

        if(key==*(int*)node->key)
            return *(bool*)node->value;
        

        node = node->next;
        while(node!=nullptr) {
            if(key==*(int*)node->key)
                return *(bool*)node->value;
            node = node->next;
        }
        
        LogErrorC(-1, "Key " + std::to_string(key) + " not found.");
        std::exit(0);
    }

    template<>
    bool get(float key) {
        int hash_slot = float_hash(key) % capacity;

        DT_map_node *node = nodes[hash_slot];

        if (node==nullptr) {
            LogErrorC(-1, "Key " + std::to_string(key) + " not found.");
            std::exit(0);
        }

        if(key==*(float*)node->key)
            return *(bool*)node->value;
        

        node = node->next;
        while(node!=nullptr) {
            if(key==*(float*)node->key)
                return *(bool*)node->value;
            node = node->next;
        }
        
        LogErrorC(-1, "Key " + std::to_string(key) + " not found.");
        std::exit(0);
    }

};


struct map_get_any {
    void *ptr;
    int has;
};
struct map_get_int {
    int val;
    int has;
};



void map_node_Clean_Up(void *ptr, int);
void map_Clean_Up(void *ptr, int);
