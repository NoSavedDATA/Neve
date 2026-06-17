#include <algorithm>
#include <random>
#include <vector>
#include <unistd.h>

#include "../codegen/random.h"

#include "../compiler_frontend/logging_execution.h"
#include "../compiler_frontend/logging_v.h"
#include "../mangler/scope_struct.h"
#include "../pool/include.h"
#include "array.h"
#include "list.h"
#include "include.h"

DT_array::DT_array() {}


void DT_array::New(Scope_Struct *ctx, int size, int elem_size, int tid, uint16_t type) {
    // std::cout << "New of size " << size << "\n";
    ctx->stw_wait();
    __atomic_store_n(&this->virtual_size, size, __ATOMIC_RELEASE);
    __atomic_store_n(&this->elem_size, elem_size, __ATOMIC_RELEASE);
    __atomic_store_n(&this->type, type, __ATOMIC_RELEASE);

    size = ((size + 7) / 8)*8;
    if (size<8)
        size = 8;
    __atomic_store_n(&this->size, size, __ATOMIC_RELEASE);

    __atomic_store_n(&this->data, cache_pop(size*elem_size, tid), __ATOMIC_RELEASE);
}

void DT_array::New(Scope_Struct *ctx, int size, int tid, uint16_t type) {
    ctx->stw_wait();
    __atomic_store_n(&this->virtual_size, size, __ATOMIC_RELEASE);
    __atomic_store_n(&this->elem_size, 8, __ATOMIC_RELEASE);
    __atomic_store_n(&this->type, type, __ATOMIC_RELEASE);

    size = ((size + 7) / 8)*8;
    if (size<8)
        size = 8;
    __atomic_store_n(&this->size, size, __ATOMIC_RELEASE);
    
    __atomic_store_n(&this->data, cache_pop(size*elem_size, tid), __ATOMIC_RELEASE);
}


extern "C" DT_array *array_Create(Scope_Struct *scope_struct, uint16_t elem_type) { 
  int elem_size;

  if(data_type_to_size.count(elem_type)>0)
      elem_size = data_type_to_size[elem_type];
  else
      elem_size = 8;

  DT_array *vec = newT<DT_array>(scope_struct, "array");
  vec->New(scope_struct, 8, elem_size, scope_struct->thread_id, elem_type);
  __atomic_store_n(&vec->virtual_size, 0, __ATOMIC_RELEASE);
  return vec;
}

extern "C" DT_array *array_clone(Scope_Struct *scope_struct, DT_array *v) { 
    
    DT_array *ret = newT<DT_array>(scope_struct, "array");

    __atomic_store_n(&ret->virtual_size, v->virtual_size, __ATOMIC_RELEASE);
    __atomic_store_n(&ret->size, v->size, __ATOMIC_RELEASE);
    __atomic_store_n(&ret->elem_size, v->elem_size, __ATOMIC_RELEASE);
    __atomic_store_n(&ret->type, v->type, __ATOMIC_RELEASE);
    
    int data_size = v->size*v->elem_size;
    __atomic_store_n(&ret->data, cache_pop(data_size, scope_struct->thread_id), __ATOMIC_RELEASE);

    memcpy(ret->data, v->data, data_size);

    return ret;
}

extern "C" DT_array *array_slice(Scope_Struct *scope_struct, DT_array *vec,\
                uint16_t elem_type, int start, int end) { 
  int vec_size = __atomic_load_n(&vec->virtual_size, __ATOMIC_ACQUIRE);
  if (end>vec_size)
      LogErrorQ(scope_struct->code_line, "Slice with end idx " + std::to_string(end) + " out of bounds. Array has size " + std::to_string(vec_size));
  if (start>end)
      LogErrorQ(scope_struct->code_line, "Array slice end \"" + std::to_string(end) + "\" is smaller than start: " + std::to_string(start));

  int elem_size, size = end-start;
  if(data_type_to_size.count(elem_type)>0)
      elem_size = data_type_to_size[elem_type];
  else
      elem_size = 8;

  DT_array *new_vec = newT<DT_array>(scope_struct, "array");
  new_vec->New(scope_struct, size, elem_size, scope_struct->thread_id, elem_type);

  void *data = __atomic_load_n(&vec->data, __ATOMIC_ACQUIRE);
  void *new_data = __atomic_load_n(&new_vec->data, __ATOMIC_ACQUIRE);
  memcpy(new_data, (char*)data+start*elem_size, size*elem_size);
  return new_vec;
}

extern "C" void *array_pop(Scope_Struct *scope_struct, DT_array *v) { 
    int virtual_size = v->virtual_size;

    void *ret = ((void**)v->data)[virtual_size-1];
    __atomic_store_n(&v->virtual_size, virtual_size-1, __ATOMIC_RELEASE);

    return ret;
}

void array_Clean_Up(void *data_ptr, int tid) {
    DT_array *array = static_cast<DT_array *>(data_ptr);
    // cache_push(array->data, array->size, tid);
}

extern "C" int array_size(Scope_Struct *scope_struct, DT_array *vec) {
    return vec->virtual_size;
}



extern "C" int array_bad_idx(int line, int idx, int size) {
    LogErrorC(line, "Tried to index array at " + std::to_string(idx) + ", but the array size is: " + std::to_string(size));
    std::exit(0);
    return 0;
}


DT_array_retire::DT_array_retire(void *data, int size, int tid)
            : data(data), size(size), tid(tid) {}


extern "C" void array_double_size(Scope_Struct *scope_struct, DT_array *vec) {
    int tid = scope_struct->thread_id;
    void *data = __atomic_load_n(&vec->data, __ATOMIC_ACQUIRE);
    int vsize = __atomic_load_n(&vec->virtual_size, __ATOMIC_ACQUIRE);
    int elem_size = __atomic_load_n(&vec->elem_size, __ATOMIC_ACQUIRE);
    int old_size = vsize*elem_size;
    int vec_size = old_size*4;

    void *new_data = cache_pop(vec_size, tid);
    memcpy(new_data, data, old_size);

    {
        scope_struct->stw_wait();
        scope_struct->gc->retire_arr(data, old_size, tid);
    }

    __atomic_store_n(&vec->data, new_data, __ATOMIC_RELEASE);
    int size = __atomic_load_n(&vec->size, __ATOMIC_ACQUIRE);
    __atomic_store_n(&vec->size, size*4, __ATOMIC_RELEASE);

    // std::cout << "double size " << scope_struct << ", " << vec << "\n";
    // int tid = scope_struct->thread_id;
    // int old_size = vec->size*vec->elem_size;
    // int vec_size = old_size*2;
    // std::cout << "size " << old_size << ", " << vec_size << ", elem " << vec->elem_size << ", size " << vec->size << "\n";

    // // void *new_data = (void*)malloc(vec_size);
    // void *new_data = cache_pop(vec_size, tid);
    // memcpy(new_data, vec->data, old_size);

    // vec->data = new_data;
    // vec->size *= 2;
    // cache_push(vec->data, old_size, tid);
    // // free(vec->data);
}

extern "C" float array_clear(Scope_Struct *scope_struct, DT_array *vec) {
    scope_struct->stw_wait();
    __atomic_store_n(&vec->virtual_size, 0, __ATOMIC_RELEASE);
    return 0;
}


extern "C" DT_array *array_int_NewVec(Scope_Struct *scope_struct, int first, ...) { 
  int elem_size = 4;

  DT_array *vec = newT<DT_array>(scope_struct, "array");
  vec->New(scope_struct, 8, elem_size, scope_struct->thread_id, 2);
  __atomic_store_n(&vec->virtual_size, 0, __ATOMIC_RELEASE);

  int *data = (int*)__atomic_load_n(&vec->data, __ATOMIC_ACQUIRE);

  int x = first;
  va_list args;
  va_start(args, x);

  int idx = 0;
  do {
    __atomic_store_n(&data[idx], x, __ATOMIC_RELEASE);
    idx++;
    int size = __atomic_load_n(&vec->virtual_size, __ATOMIC_ACQUIRE);
    __atomic_store_n(&vec->virtual_size, size+1, __ATOMIC_RELEASE);
    if (vec->virtual_size >= vec->size) {
        array_double_size(scope_struct, vec);
        int *data = (int*)__atomic_load_n(&vec->data, __ATOMIC_ACQUIRE);
    }
    x = va_arg(args, int);
  } while(x!=TERMINATE_VARARG);
  va_end(args);
  // std::cout << "new vec: " << vec << "\n";
  return vec;
}

extern "C" DT_array *array_void_NewVec(Scope_Struct *scope_struct, void *first, ...) { 
  int elem_size = 8;

  DT_array *vec = newT<DT_array>(scope_struct, "array");
  vec->New(scope_struct, 8, elem_size, scope_struct->thread_id, 2);
  __atomic_store_n(&vec->virtual_size, 0, __ATOMIC_RELEASE);

  void **data = (void**)__atomic_load_n(&vec->data, __ATOMIC_ACQUIRE);

  void *x = first;
  va_list args;
  va_start(args, x);

  int idx = 0;
  do {
    __atomic_store_n(&data[idx], x, __ATOMIC_RELEASE);
    idx++;
    int size = __atomic_load_n(&vec->virtual_size, __ATOMIC_ACQUIRE);
    __atomic_store_n(&vec->virtual_size, size+1, __ATOMIC_RELEASE);
    if (vec->virtual_size >= vec->size) {
        array_double_size(scope_struct, vec);
        void **data = (void**)__atomic_load_n(&vec->data, __ATOMIC_ACQUIRE);
    }
    x = va_arg(args, void*);
  } while(x!=nullptr);
  va_end(args);
  // std::cout << "new vec: " << vec << "\n";
  return vec;
}


extern "C" float array_print_int(Scope_Struct *scope_struct, DT_array *vec) {
    std::cout << std::dec;
    int *ptr = static_cast<int*>(vec->data);
    int size = vec->virtual_size;

    std::cout << "[";
    for (int i=0; i<size-1; ++i)
        std::cout << ptr[i] << ",";
    std::cout << ptr[size-1] << "]\n";
    return 0;
}

extern "C" float array_print_char(Scope_Struct *scope_struct, DT_array *vec) {
    std::cout << std::dec;
    char *ptr = static_cast<char*>(vec->data);
    int size = vec->virtual_size;

    std::cout << "[";
    for (int i=0; i<size-1; ++i)
        std::cout << ptr[i] << ",";
    std::cout << ptr[size-1] << "]\n";
    // std::cout << "[";
    // for (int i=0; i<size-1; ++i)
    //     std::cout << int(ptr[i]) << ",";
    // std::cout << int(ptr[size-1]) << "]\n\n";
    return 0;
}

extern "C" DT_array *arange_int(Scope_Struct *scope_struct, int begin, int end) {
    DT_array *vec = newT<DT_array>(scope_struct, "array");
    vec->New(scope_struct, end-begin, 4, scope_struct->thread_id, 2);
    __atomic_store_n(&vec->virtual_size, end-begin, __ATOMIC_RELEASE);

    int *ptr = static_cast<int*>(vec->data);
    
    int c=0;
    for(int i=begin; i<end; ++i) {
        ptr[c] = i;
        c++;
    }

    return vec; 
} 


extern "C" DT_array *zeros_int(Scope_Struct *scope_struct, int N) {
    DT_array *vec = newT<DT_array>(scope_struct, "array");
    vec->New(scope_struct, N, 4, scope_struct->thread_id, 2);
    __atomic_store_n(&vec->virtual_size, N, __ATOMIC_RELEASE);

    int *ptr = static_cast<int*>(vec->data);
    
    int c=0;
    for(int i=0; i<N; ++i) {
        ptr[c] = 0;
        c++;
    }

    return vec; 
} 



extern "C" DT_array *randint_array(Scope_Struct *scope_struct, int size, int min_val, int max_val) {
    DT_array *vec = newT<DT_array>(scope_struct, "array");
    vec->New(scope_struct, size, 4, scope_struct->thread_id, 2);

    std::uniform_int_distribution<int> dist(min_val, max_val);

    int *ptr = static_cast<int*>(vec->data);
    for (int i = 0; i < size; ++i)
        ptr[i] = dist(MAIN_PRNG);
    

    return vec;
}


extern "C" DT_array *ones_int(Scope_Struct *scope_struct, int N) {
    DT_array *vec = newT<DT_array>(scope_struct, "array");
    vec->New(scope_struct, N, 4, scope_struct->thread_id, 2);

    int *ptr = static_cast<int*>(vec->data);
    
    int c=0;
    for(int i=0; i<N; ++i)
    {
        ptr[c] = 1;
        c++;
    }

    return vec;
}

extern "C" DT_array *array_int_add(Scope_Struct *scope_struct, DT_array *array, int x) {
    DT_array *new_array = newT<DT_array>(scope_struct, "array");
    new_array->New(scope_struct, array->virtual_size, 4, scope_struct->thread_id, 2);
    
    int *data = static_cast<int *>(array->data);
    int *new_data = static_cast<int *>(new_array->data);
    for (int i=0; i<array->virtual_size; ++i) {
        new_data[i] = data[i] + x;
    }

    return new_array;
}



extern "C" DT_array *randfloat_array(Scope_Struct *scope_struct, int size, float min_val, float max_val) {
    DT_array *vec = newT<DT_array>(scope_struct, "array");
    vec->New(scope_struct, size,4, scope_struct->thread_id, 3);

    std::uniform_real_distribution<float> dist(min_val, max_val);

    float *ptr = static_cast<float*>(vec->data);
    for (int i = 0; i < size; ++i)
        ptr[i] = dist(MAIN_PRNG);
    

    return vec;
}

extern "C" int array_print_float(Scope_Struct *scope_struct, DT_array *vec) {
    float *ptr = static_cast<float*>(vec->data);
    int size = vec->virtual_size;

    std::cout << "[";
    for (int i=0; i<size-1; ++i)
        printf("%.3f, ",ptr[i]);
    printf("%.3f]\n",ptr[size-1]);
    return 0;
}


extern "C" DT_array *arange_float(Scope_Struct *scope_struct, float begin, float end) {
    DT_array *vec = newT<DT_array>(scope_struct, "array");
    vec->New(scope_struct, end-begin, 4, scope_struct->thread_id, 3);

    float *ptr = static_cast<float*>(vec->data);
    
    int c=0;
    for(int i=begin; i<end; ++i)
    {
        ptr[c] = i;
        c++;
    }

    return vec; 
} 

extern "C" DT_array *zeros_float(Scope_Struct *scope_struct, int N) {
    DT_array *vec = newT<DT_array>(scope_struct, "array");
    vec->New(scope_struct, N, 4, scope_struct->thread_id, 3);

    float *ptr = static_cast<float*>(vec->data);
    
    int c=0;
    for(int i=0; i<N; ++i)
    {
        ptr[c] = 0.0f;
        c++;
    }

    return vec; 
}


extern "C" DT_array *ones_float(Scope_Struct *scope_struct, int N) {
    DT_array *vec = newT<DT_array>(scope_struct, "array");
    vec->New(scope_struct, N, 4, scope_struct->thread_id, 3);

    float *ptr = static_cast<float*>(vec->data);
    
    int c=0;
    for(int i=0; i<N; ++i)
    {
        ptr[c] = 1.0f;
        c++;
    }

    return vec; 
}

extern "C" int array_sum_int(Scope_Struct *scope_struct, DT_array *arr) {
    int *data = static_cast<int*>(arr->data);
    int len = arr->virtual_size;

    int sum=0;
    for (int i = 0; i < len; ++i)
        sum += data[i];

    return sum;
}
extern "C" int array_prod_int(Scope_Struct *scope_struct, DT_array *arr) {
    int *data = static_cast<int*>(arr->data);
    int len = arr->virtual_size;

    int prod=1;
    for (int i = 0; i < len; ++i)
        prod *= data[i];
    return prod;
}




extern "C" DT_array *array_Split_Parallel(Scope_Struct *scope_struct, DT_array *vec) {
    int threads_count = scope_struct->asyncs_count;
    int thread_id = scope_struct->thread_id-1;

    int vec_size = vec->virtual_size;
    int elem_size = vec->elem_size;
    int segment_size;

    segment_size = ceilf(vec_size/(float)threads_count);


    int size = segment_size;
    if((thread_id+1)==threads_count)
      size = vec_size - segment_size*thread_id;
      

    int copy_size;
    if(segment_size*(thread_id+1)>vec_size) 
        copy_size = (vec_size - segment_size*thread_id)*elem_size;
    else
        copy_size = segment_size*elem_size;


    DT_array *out_vector = newT<DT_array>(scope_struct, "array");
    out_vector->New(scope_struct, size, elem_size, scope_struct->thread_id, vec->type);

    
    memcpy(out_vector->data,
           static_cast<char*>(vec->data) + segment_size*thread_id*elem_size,
           copy_size);

    return out_vector;
}

// extern "C" int array_print_str(Scope_Struct *scope_struct, DT_array *arr) {
//     char **data = static_cast<char**>(arr->data);
//     int len = arr->virtual_size;
//     std::cout << data[0];
//     for (int i=1; i<len; ++i)
//         std::cout << ", " << data[i];
//     std::cout << "\n";
//     return 0;
// }
extern "C" int array_print_str(Scope_Struct *scope_struct, DT_array *arr) {
    DT_str *data = static_cast<DT_str*>(arr->data);
    int len = arr->virtual_size;

    int offset = 0, elem_size=data[0].size;
    memcpy(scope_struct->print_buffer, data[0].str, elem_size);
    offset+=elem_size;
    for (int i = 1; i < len; ++i) {
        scope_struct->print_buffer[offset++] = ',';
        elem_size = data[i].size;
        if(offset+elem_size>PrintBufferSize) {
            write(1, scope_struct->print_buffer, offset);
            offset=0;
        }
        memcpy(scope_struct->print_buffer+offset, data[i].str, elem_size);
        offset+=elem_size;
    }
    write(1, scope_struct->print_buffer, offset);
    scope_struct->print_buffer[0] = '\n';
    write(1, scope_struct->print_buffer, 1);
    return 0;
}

extern "C" int array_shuffle_str(Scope_Struct *ctx, DT_array *arr) {
    DT_str *data = static_cast<DT_str*>(arr->data);
    int len = arr->virtual_size;
    DT_str *new_data = (DT_str*)cache_pop(arr->size*arr->elem_size, ctx->thread_id);

    // std::cout << "arr " << arr << "\n";
    // std::cout << "data " << data << "\n";
    // std::cout << "GOT LEN " << len << "\n";
    
    std::vector<int> indices(len);
    for (int i = 0; i < len; ++i)
        indices[i] = i;
    thread_local std::mt19937 rng(std::random_device{}());
    std::shuffle(indices.begin(), indices.end(), rng);

    for (int i = 0; i < len; ++i) {
        // __atomic_store_n(&arr->data, cache_pop(arr->size*arr->elem_size, ctx->thread_id), __ATOMIC_RELEASE);
        new_data[indices[i]] = data[i];
    }
    
    __atomic_store_n(&arr->data, (void*)new_data, __ATOMIC_RELEASE);

    return 0;
}





extern "C" int hash_array_int(Scope_Struct *ctx, DT_array *arr) {
    int h = 2166136261u; // FNV offset
    int *data = (int*)arr->data;
    int size = std::min(arr->virtual_size, 8);

    for (int i=0; i<size; ++i) {
        h ^= data[i];
        h *= 16777619u;
    }
    return h;
}

extern "C" bool array_eq_int(Scope_Struct *ctx, DT_array *arr_x, DT_array *arr_y) {
    if (arr_x->virtual_size != arr_y->virtual_size)
        return false;

    return memcmp(
        arr_x->data,
        arr_y->data,
        arr_x->virtual_size * sizeof(int)
    ) == 0;
    // return true;
}
