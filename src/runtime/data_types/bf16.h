#pragma once
#include <cstdint>


inline uint16_t float_to_bf16_inline(float x) {
    uint32_t u;
    memcpy(&u, &x, sizeof(u));

    // Round-to-nearest-even
    u += 0x7FFF + ((u >> 16) & 1);


    return uint16_t(u >> 16);
}

inline float bf16_to_float_inline(uint16_t bits) {
    uint32_t u = uint32_t(bits) << 16;
    float x;
    memcpy(&x, &u, sizeof(x));
    return x;
}

