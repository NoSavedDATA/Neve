#include <cassert>
#include <cstddef>
#include <cstring>


#include "../data_types/str_view.h"
#include "../pool/include.h"
#include "../threads/cas.h"
#include "channels.h"



Channel::Channel() {}

void Channel::New(Scope_Struct *scope_struct, uint16_t type, int buffer_size) {
    // std::unique_lock<std::mutex> lock(scope_struct->gc->arena->sweep_mtx);
    this->buffer_size = buffer_size;
    __atomic_store_n(&this->buffer_size, buffer_size, __ATOMIC_RELEASE);
    this->type = type;
    int tid = scope_struct->thread_id;

    int elem_size;
    if(data_type_to_size.count(type)>0)
      elem_size = data_type_to_size[type];
    else
      elem_size = 8;

    __atomic_store_n(&data, cache_pop(elem_size*buffer_size, scope_struct->thread_id), __ATOMIC_RELEASE);

    seq = (size_t*)malloc(buffer_size*sizeof(size_t));
    for (int i=0;i<buffer_size;i++)
        seq[i] = i;

    __atomic_store_n(&init, true, __ATOMIC_RELEASE);
}


extern "C" void *channel_Create(Scope_Struct *scope_struct, uint16_t type, int buffer_size) {
    Channel *ch = newT<Channel>(scope_struct, "channel");
    ch->New(scope_struct, type, buffer_size);
    return ch;
}





// extern "C" void *void_channel_message(Scope_Struct *scope_struct, void *ptr, Channel *ch) {    
//     std::unique_lock<std::mutex> lock(ch->mtx);

//     ch->cv.wait(lock, [&]{ return ch->terminated || ch->data_list->data->size() > 0; } );    
//     if(ch->terminated)
//         return ptr;

//     void *ret = ch->data_list->unqueue<void*>();
//     ch->cv.notify_all();

//     return ret;
// }

// extern "C" float channel_void_message(Scope_Struct *scope_struct, Channel *ch, void *ptr) {
//     std::unique_lock<std::mutex> lock(ch->mtx);

//     ch->cv.wait(lock, [&]{ return ch->terminated || ch->data_list->data->size() < ch->buffer_size; } );    
//     if(ch->terminated)
//         return -1;

//     ch->data_list->append(std::any(ptr), "any");
//     ch->cv.notify_all();
    
//     return 0;
// }




// str x <- ch
extern "C" DT_str str_channel_message(Scope_Struct *scope_struct, void *ptr, Channel *ch) {    
    DT_str *data = (DT_str*)ch->data;
    
    int backoff_us = 1;
    while(!ch->terminated) {
        size_t pos = __atomic_load_n(&ch->head, __ATOMIC_RELAXED) ;
        int ring_pos = pos % ch->buffer_size;

        size_t seq = __atomic_load_n(&ch->seq[ring_pos], __ATOMIC_ACQUIRE);

        intptr_t diff = (intptr_t)seq - (intptr_t)(pos+1);
        if (diff==0) {
            if (__atomic_compare_exchange_n(
                            &ch->head, &pos, pos+1,
                            false,
                            __ATOMIC_RELAXED,
                            __ATOMIC_RELAXED
                        )) {

                DT_str x;
                __atomic_load(&data[ring_pos], &x, __ATOMIC_RELAXED); 
                __atomic_store_n(&ch->seq[ring_pos], pos + ch->buffer_size, __ATOMIC_RELEASE);
                return x;
            }
        } 
        cas_sleep(backoff_us);
    }
}


// ch <- msg
extern "C" int channel_str_message(Scope_Struct *scope_struct, Channel *ch, DT_str str) {

    DT_str *data = (DT_str*)ch->data;
    
    int backoff_us = 1;
    while(!ch->terminated) {
        size_t pos = __atomic_load_n(&ch->tail, __ATOMIC_RELAXED) ;

        int ring_pos = pos % ch->buffer_size;

        size_t seq = __atomic_load_n(&ch->seq[ring_pos], __ATOMIC_ACQUIRE);

        intptr_t diff = (intptr_t)seq - (intptr_t)pos;
        if (diff==0) {
            if (__atomic_compare_exchange_n(
                            &ch->tail, &pos, pos+1,
                            false,
                            __ATOMIC_RELAXED,
                            __ATOMIC_RELAXED
                        )) {
                DT_str tmp(str.str, str.size);
                __atomic_store(&data[ring_pos], &tmp, __ATOMIC_RELEASE);
                __atomic_store_n(&ch->seq[ring_pos], pos+1, __ATOMIC_RELEASE);

                return 0;
            }
        }
        cas_sleep(backoff_us);
    }
    
    return 0;
}

extern "C" DT_str str_channel_Idx(
        Scope_Struct *scope_struct,
        Channel *ch,
        int idx) {
    DT_str *data = (DT_str*)ch->data;

    int backoff_us = 1;
    while (!ch->terminated) {
        size_t head = __atomic_load_n(&ch->head, __ATOMIC_ACQUIRE);
        size_t tail = __atomic_load_n(&ch->tail, __ATOMIC_ACQUIRE);

        size_t size = tail - head;

        if ((size_t)idx >= size) {
            cas_sleep(backoff_us);
            continue;
        }

        size_t pos = head + idx;
        int ring_pos = pos % ch->buffer_size;

        size_t seq = __atomic_load_n(
            &ch->seq[ring_pos],
            __ATOMIC_ACQUIRE
        );

        // slot contains valid data?
        if (seq == pos + 1) {
            DT_str ret;
            __atomic_load(
                &data[ring_pos],
                &ret,
                __ATOMIC_ACQUIRE
            );
            return ret;
        }
    }

    return DT_str(nullptr,0);
}


extern "C" void str_channel_terminate(Scope_Struct *scope_struct, Channel *ch) {
    {
        std::unique_lock<std::mutex> lock(ch->mtx);
        ch->terminated = true;
    }
    ch->cv.notify_all();  // wake up all waiting senders/receivers
}


extern "C" int str_channel_alive(Scope_Struct *scope_struct, Channel *ch) {
    return !ch->terminated;
}















extern "C" float float_channel_terminate(Scope_Struct *scope_struct, Channel *ch) {
    {
        std::unique_lock<std::mutex> lock(ch->mtx);
        ch->terminated = true;
    }
    ch->cv.notify_all();  // wake up all waiting senders/receivers
    return 0;
}
extern "C" int float_channel_alive(Scope_Struct *scope_struct, Channel *ch) {
    return !ch->terminated;
}






// int x <- ch
extern "C" int int_channel_message(Scope_Struct *scope_struct, void *ptr, Channel *ch) {    
    int *data = (int*)ch->data;
    
    int backoff_us = 1;
    while(!ch->terminated) {
        size_t pos = __atomic_load_n(&ch->head, __ATOMIC_RELAXED) ;
        int ring_pos = pos % ch->buffer_size;

        size_t seq = __atomic_load_n(&ch->seq[ring_pos], __ATOMIC_ACQUIRE);

        intptr_t diff = (intptr_t)seq - (intptr_t)(pos+1);
        if (diff==0) {
            if (__atomic_compare_exchange_n(
                            &ch->head, &pos, pos+1,
                            false,
                            __ATOMIC_RELAXED,
                            __ATOMIC_RELAXED
                        )) {

                int x = __atomic_load_n(&data[ring_pos], __ATOMIC_RELAXED); 
                __atomic_store_n(&ch->seq[ring_pos], pos + ch->buffer_size, __ATOMIC_RELEASE);
                return x;
            }
        } 
        cas_sleep(backoff_us);
    }
}


// ch <- msg
extern "C" float channel_int_message(Scope_Struct *scope_struct, Channel *ch, int x) {
    int *data = (int*)ch->data;
    
    int backoff_us = 1;
    while(!ch->terminated) {
        size_t pos = __atomic_load_n(&ch->tail, __ATOMIC_RELAXED) ;
        int ring_pos = pos % ch->buffer_size;

        size_t seq = __atomic_load_n(&ch->seq[ring_pos], __ATOMIC_ACQUIRE);

        intptr_t diff = (intptr_t)seq - (intptr_t)pos;
        if (diff==0) {
            if (__atomic_compare_exchange_n(
                            &ch->tail, &pos, pos+1,
                            false,
                            __ATOMIC_RELAXED,
                            __ATOMIC_RELAXED
                        )) {
                __atomic_store_n(&data[ring_pos], x, __ATOMIC_RELEASE);
                __atomic_store_n(&ch->seq[ring_pos], pos+1, __ATOMIC_RELEASE);
                return 0;
            }
        }
        cas_sleep(backoff_us);
    }
    
    return 0;
}

extern "C" int int_channel_Idx(
        Scope_Struct *scope_struct,
        Channel *ch,
        int idx) {
    int *data = (int*)ch->data;

    int backoff_us = 1;
    while (!ch->terminated) {
        size_t head = __atomic_load_n(&ch->head, __ATOMIC_ACQUIRE);
        size_t tail = __atomic_load_n(&ch->tail, __ATOMIC_ACQUIRE);

        size_t size = tail - head;

        if ((size_t)idx >= size) {
            cas_sleep(backoff_us);
            continue;
        }

        size_t pos = head + idx;
        int ring_pos = pos % ch->buffer_size;

        size_t seq = __atomic_load_n(
            &ch->seq[ring_pos],
            __ATOMIC_ACQUIRE
        );

        // slot contains valid data?
        if (seq == pos + 1) {
            return __atomic_load_n(
                &data[ring_pos],
                __ATOMIC_ACQUIRE
            );
        }
    }

    return -1;
}


extern "C" int int_channel_sum(Scope_Struct *scope_struct, Channel *ch) {
    std::unique_lock<std::mutex> lock(ch->mtx);

    ch->cv.wait(lock, [&]{ return ch->terminated || ch->size <= ch->buffer_size; } );
    if(ch->terminated)
        return -1;

    int *data = (int*)ch->data;

    int sum=0;
    for(int i=0; i<ch->buffer_size; ++i)
        sum += data[i];

    ch->cv.notify_all();
    
    return sum;
}



extern "C" float int_channel_terminate(Scope_Struct *scope_struct, Channel *ch) {
    {
        std::unique_lock<std::mutex> lock(ch->mtx);
        ch->terminated = true;
    }
    ch->cv.notify_all();  // wake up all waiting senders/receivers
    return 0;
}

extern "C" bool int_channel_alive(Scope_Struct *scope_struct, Channel *ch) {
    return !ch->terminated;
}


void channel_Clean_Up(void *ptr, int tid) {
}
