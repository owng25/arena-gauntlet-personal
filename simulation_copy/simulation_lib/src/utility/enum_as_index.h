#pragma once

#include <cassert>
#include <cstddef>
#include <type_traits>

namespace simulation
{

// This type trait helps to interpret enumeration as an index
// and determine if this freature is supported by specified enumeration.
template <typename T>
struct EnumAsIndex
{
    // Specializations of this template should set this to true
    // to mark enumeration as one that supports using it as an index
    static constexpr bool kImplemented = false;

    // Specializations of this template must implement this function
    // which converts enumeration value to an index
    static constexpr size_t ToIndex(T)
    {
        return 0;
    }

    // Specializations of this template must implement this function
    // which converts indices in range from [0, GetEntriesCount] to enum value
    static constexpr size_t ToEnum(size_t)
    {
        return 0;
    }

    // Specializations of this template must implement this function
    // which returns enumeration entries count (i.e. required array size)
    static constexpr size_t GetEntriesCount()
    {
        return 0;
    }
};

// This is a default implementation of EnumAsIndex type trait.
// `begin` and `end` parameters specify indexed rage of enumeration.
// It asserts when someone attempts to use T < begin || T > end value as an index.
template <typename T, T begin, T end>
struct EnumAsIndex_Default
{
    // Underlying type of enumeration
    using U = std::underlying_type_t<T>;

    static constexpr bool kImplemented = true;
    static constexpr U kBegin = static_cast<U>(begin);
    static constexpr U kEnd = static_cast<U>(end);

    static_assert(kBegin <= kEnd);
    static_assert(std::is_enum_v<T>);

    static constexpr size_t ToIndex(T value)
    {
        const U underlying = static_cast<U>(value);
        assert((underlying >= kBegin) && (underlying < kEnd));
        return static_cast<size_t>(underlying - kBegin);
    }

    static constexpr T ToEnum(size_t index)
    {
        const U underlying = kBegin + static_cast<U>(index);
        assert(underlying < kEnd);
        return static_cast<T>(underlying);
    }

    static constexpr size_t GetEntriesCount()
    {
        return static_cast<size_t>(kEnd - kBegin);
    }
};

template <typename T, typename Enable = std::enable_if_t<EnumAsIndex<T>::kImplemented>>
constexpr T IndexToEnum(size_t index)
{
    return EnumAsIndex<T>::ToEnum(index);
}

template <typename T, typename Enable = std::enable_if_t<EnumAsIndex<T>::kImplemented>>
constexpr size_t EnumToIndex(T value)
{
    return EnumAsIndex<T>::ToIndex(value);
}

template <typename T, typename Enable = std::enable_if_t<EnumAsIndex<T>::kImplemented>>
constexpr size_t GetEnumEntriesCount()
{
    return EnumAsIndex<T>::GetEntriesCount();
}

// This macro allows using enumeration as index by implementing EnumAsIndex trait.
// This one is for enumerations without kNone entry.
#define ILLUVIUM_ENUM_AS_INDEX(Enum)                                                       \
    template <>                                                                            \
    struct EnumAsIndex<Enum> : EnumAsIndex_Default<Enum, static_cast<Enum>(0), Enum::kNum> \
    {                                                                                      \
    }

// This macro allows using enumeration as index by implementing EnumAsIndex trait.
// This one is for enumerations with kNone entry.
#define ILLUVIUM_ENUM_AS_INDEX_IGNORE_FIRST_VALUE(Enum)                                    \
    template <>                                                                            \
    struct EnumAsIndex<Enum> : EnumAsIndex_Default<Enum, static_cast<Enum>(1), Enum::kNum> \
    {                                                                                      \
    }

}  // namespace simulation
