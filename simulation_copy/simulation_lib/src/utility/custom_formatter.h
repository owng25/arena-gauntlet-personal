#pragma once

#include <cassert>
#include <filesystem>

#include "utility/fmtlib.h"

namespace simulation
{

// This type trait if we have defined an fmt lib formatting specialization for type T
// The only use of it at the moment is to automatically create operator<< for all types
// for simulation and do go into ambiguity with other types =
template <typename T>
struct HasSimFormatter
{
    static constexpr bool kValue = false;
};

// Make an output operator for printing values to std::ostream for formattable types
template <typename T, typename Enable = std::enable_if_t<simulation::HasSimFormatter<T>::kValue>>
std::ostream& operator<<(std::ostream& out, const T& value)
{
    fmt::format_to(std::ostream_iterator<char>(out), "{}", value);
    return out;
};

// This structure used as base class for other formatters so that they don't have
// to write parse method. This method parses format modifiers from format string
// which we usually don't need.
struct FormatterWithEmptyParse
{
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin())
    {
        return ctx.end();
    }
};

// Formatter template for enumerations that have function to convert values to string_view
template <typename T, auto to_string_function>
struct DefaultEnumFormatter : FormatterWithEmptyParse
{
    auto format(T value, fmt::format_context& ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{}", to_string_function(value));
    }
};

}  // namespace simulation

// This makes a specialization for fmt::formatter which calls TypeName::Method to write type to text stream
#define ILLUVIUM_MAKE_STRUCT_FORMATTER_NO_NAMESPACE(TypeName, Method)                             \
    template <>                                                                                   \
    struct fmt::formatter<TypeName> : simulation::FormatterWithEmptyParse                         \
    {                                                                                             \
        auto format(const TypeName& value, fmt::format_context& ctx) const -> decltype(ctx.out()) \
        {                                                                                         \
            value.Method(ctx);                                                                    \
            return ctx.out();                                                                     \
        }                                                                                         \
    };                                                                                            \
                                                                                                  \
    template <>                                                                                   \
    struct simulation::HasSimFormatter<TypeName>                                                  \
    {                                                                                             \
        static constexpr bool kValue = true;                                                      \
    }

// This makes a specialication for fmt::formatter which calls TypeName::Method to write type to text stream
#define ILLUVIUM_MAKE_STRUCT_FORMATTER(TypeName, Method) \
    ILLUVIUM_MAKE_STRUCT_FORMATTER_NO_NAMESPACE(simulation::TypeName, Method)

// Make std::filesystem::path formattable.
template <>
struct fmt::formatter<std::filesystem::path> : formatter<std::string_view>
{
    template <typename FormatContext>
    auto format(const std::filesystem::path& path, FormatContext& ctx) const
    {
        return formatter<std::string_view>::format(path.string(), ctx);
    }
};
