#pragma once

#include <type_traits>

#define ILLUVIUM_ENSURE_ENUM_SIZE(Type, PreviousSize)                          \
    static_assert(                                                             \
        static_cast<std::underlying_type_t<Type>>(Type::kNum) == PreviousSize, \
        "Enumeration changed, update this place")
