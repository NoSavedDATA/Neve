#pragma once

#include <array>
#include <cstdint>

struct PtrInfo {
    uint16_t offset;
    uint16_t type;

};

struct TypeInfo {
    uint16_t pointers_count;
    PtrInfo ptr_info[];
};

extern std::array<TypeInfo *, 4096> type_info;
