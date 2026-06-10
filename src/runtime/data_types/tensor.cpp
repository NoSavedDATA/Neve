#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <string>

#include "../codegen/string.h"
#include "../common/extension_functions.h"
#include "../mangler/scope_struct.h"
#include "../pool/include.h"
#include "include.h"




void tensor_Clean_Up(void *data_ptr, int) {
    DT_tensor *tensor = (DT_tensor*)data_ptr;
    std::cout << "tensor clean up" << "\n";
    std::cout << "tensor clean up" << tensor << "\n";
    std::cout << "tensor clean up" << tensor->data << "\n";
}
