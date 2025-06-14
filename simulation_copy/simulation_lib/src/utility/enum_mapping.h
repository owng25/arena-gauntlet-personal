
#pragma once

#include <string>

#include "utility/string.h"

namespace simulation
{
namespace enum_mapping
{
template <typename TValue>
struct NameValuePair
{
    using value_type = TValue;
    const TValue value;
    const std::string_view name;
};

// Gets the enum name from the value literal
template <typename TMapping, typename TValue>
std::string_view GetNameForValue(const TMapping& array, const TValue value, const std::string_view default_value = "")
{
    for (const auto& pair : array)
    {
        if (pair.value == value)
        {
            return pair.name;
        }
    }

    return default_value;
}

// Gets the enum literal from the name
template <typename TMapping>
constexpr typename TMapping::value_type::value_type GetValueForName(const TMapping& array, const std::string_view name)
{
    // TODO(vampy): Maybe trim the name for white spaces here
    for (const auto& pair : array)
    {
        if (pair.name == name)
        {
            return pair.value;
        }
    }

    return typename TMapping::value_type::value_type();
}

}  // namespace enum_mapping

}  // namespace simulation
