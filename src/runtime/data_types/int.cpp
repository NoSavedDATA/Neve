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



extern "C" int read_int(Scope_Struct *scope_struct) {
    int value;
    if (scanf("%d", &value) != 1) {
        fprintf(stderr, "Failed to read int\n");
        return 0; // default on failure
    }
    return value;
}


// extern "C" char *int_to_str(Scope_Struct *scope_struct, int x) {
//     // Enough for 32-bit int including sign and null terminator
//     char buffer[32];  
//     int len = std::snprintf(buffer, sizeof(buffer), "%d", x);

//     if (len < 0) return nullptr;  // snprintf error

//     char *result = allocate<char>(scope_struct, len+1, "str");
//     if (!result) return nullptr;  // malloc failed

//     std::memcpy(result, buffer, len + 1); // copy with '\0'

//     return result;
// }





template<typename T>
static int intT_to_str_buffer(T value, char *buffer) {
    if (value == std::numeric_limits<T>::min()) {
        constexpr auto min_str =
            std::numeric_limits<T>::digits == 7  ? "-128" :
            std::numeric_limits<T>::digits == 15 ? "-32768" :
                                                   "-9223372036854775808";

        constexpr int len =
            std::numeric_limits<T>::digits == 7  ? 4 :
            std::numeric_limits<T>::digits == 15 ? 6 :
                                                   20;

        std::memcpy(buffer, min_str, len);
        return len;
    }

    bool negative = value < 0;
    if (negative)
        value = -value;

    T temp = value;
    int digits = 1;

    while (temp >= 10) {
        temp /= 10;
        ++digits;
    }

    int total_len = digits + (negative ? 1 : 0);

    for (int i = 0; i < digits; ++i) {
        buffer[total_len - 1 - i] =
            static_cast<char>('0' + (value % 10));
        value /= 10;
    }

    if (negative)
        buffer[0] = '-';

    return total_len;
}

extern "C" int i64_to_str_buffer(Scope_Struct *scope_struct, int64_t value, char *buffer) {
    return intT_to_str_buffer(value, buffer);
}

extern "C" int i16_to_str_buffer(Scope_Struct *scope_struct, int16_t value, char *buffer) {
    return intT_to_str_buffer(value, buffer);
}

extern "C" int i8_to_str_buffer(Scope_Struct *scope_struct, int8_t value, char *buffer) {
    return intT_to_str_buffer(value, buffer);
}

extern "C" int int_to_str_buffer(Scope_Struct *scope_struct, int value, char *buffer) {
    return intT_to_str_buffer(value, buffer);
}








template<typename T>
static int print_bits_impl(T value) {
    using U = std::make_unsigned_t<T>;

    U u = static_cast<U>(value);

    for (int i = sizeof(T) * 8 - 1; i >= 0; --i)
        std::putchar((u >> i) & 1 ? '1' : '0');

    std::putchar('\n');
    return 0;
}

extern "C" int int_print_bits(Scope_Struct *scope_struct, int value) {
    return print_bits_impl(value);
}

extern "C" int i8_print_bits(Scope_Struct *scope_struct, int8_t value) {
    return print_bits_impl(value);
}

extern "C" int i16_print_bits(Scope_Struct *scope_struct, int16_t value) {
    return print_bits_impl(value);
}

extern "C" int i64_print_bits(Scope_Struct *scope_struct, int64_t value) {
    return print_bits_impl(value);
}


extern "C" int get_size(Scope_Struct *scope_struct, int elem_type_id) {
    if (data_type_to_size.count(elem_type_id)>0)
        return data_type_to_size[elem_type_id];
    return 8; 
}
