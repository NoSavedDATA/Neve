#pragma once


#include <condition_variable>
#include <iostream>
#include <map>
#include <mutex>
#include <string>


#include "../data_types/array.h"
#include "barrier.h"




struct DT_channel {
    int buffer_size; 
    uint16_t type;
    bool terminated=false, init=false;

    void *data=nullptr;
    size_t head=0, tail=0, size=0;
    size_t *seq;
    
    std::mutex mtx;
    std::condition_variable push_cv, pop_cv, cv;

    DT_channel();
    void New(Scope_Struct *, uint16_t, int);
};

void channel_Clean_Up(void *ptr, int);


void channel_handle_pool(Scope_Struct *scope_struct, void *ptr, char *data_name);
