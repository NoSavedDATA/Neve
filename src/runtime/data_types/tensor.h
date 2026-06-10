#pragma once

#include <iostream>
#include <vector>
#include <string>

#include "../common/extension_functions.h"
#include "../mangler/scope_struct.h"
#include "include.h"


struct DT_tensor {
    void *data;
    int type, dims_prod;
    DT_array *dims, stride;
};




void tensor_Clean_Up(void *data_ptr, int);
