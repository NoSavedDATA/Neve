#include "../mangler/scope_struct.h"
#include <cstdint>
#include "bf16.h"


extern "C" uint16_t float_to_bf16(Scope_Struct *ctx, float x) {
    // uint32_t u;
    // memcpy(&u, &x, sizeof(u));

    // // Round-to-nearest-even
    // u += 0x7FFF + ((u >> 16) & 1);


    // return uint16_t(u >> 16);

    return float_to_bf16_inline(x);
}

extern "C" float bf16_to_float(Scope_Struct *ctx, uint16_t bits) {
    // uint32_t u = uint32_t(bits) << 16;
    // float x;
    // memcpy(&x, &u, sizeof(x));
    // return x;
    return bf16_to_float_inline(bits);
}

extern "C" int bf16_to_str_buffer(Scope_Struct *scope_struct, uint16_t x, char *print_buffer) {
    // Enough for 32-bit int including sign and null terminator
    char buffer[32];  
    int len = std::snprintf(buffer, sizeof(buffer), "%f", bf16_to_float(scope_struct, x));

    if (len < 0) return 0;  // snprintf error

    std::memcpy(print_buffer, buffer, len);
    return len;
}
