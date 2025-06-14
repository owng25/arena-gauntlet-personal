#pragma once

#include <string_view>

#include "utility/fixed_point.h"
#include "utility/fmtlib.h"

namespace simulation
{

/* This is a helper for writing FromatTo methods for structures
 * It's purpose is to make structures appear in logs in the same way and eliminate
 * some boilerplate by providing tools for most common situations
 */
struct StructFormattingHelper
{
    explicit StructFormattingHelper(fmt::format_context& context) : ctx(context) {}

    // Just writes formatted text
    template <typename... Args>
    void Write(const fmt::format_string<Args...>& fmt, Args&&... args)
    {
        fmt::format_to(ctx.out(), fmt, std::forward<Args>(args)...);
    }

    // Writes a field with custom format. Adds ", " if this is not the first field
    template <typename... Args>
    void CustomField(const fmt::format_string<Args...>& fmt, Args&&... args)
    {
        if (written_first_field_)
        {
            Write(", ");
        }
        else
        {
            written_first_field_ = true;
        }
        Write(fmt, std::forward<Args>(args)...);
    }

    // The same as "CustomField" but does nothing if condition is false
    template <typename... Args>
    void CustomFieldIf(bool condition, const fmt::format_string<Args...>& fmt, Args&&... args)
    {
        if (condition)
        {
            CustomField(fmt, std::forward<Args>(args)...);
        }
    }

    // Writes field with default format and adds comma if it is not the first field written
    // For example Filed("count", 10) will yield "count = 10"
    template <typename Arg>
    void Field(const std::string_view name, const Arg& arg)
    {
        CustomField("{} = {}", name, arg);
    }

    // The same as "Field" but does nothing if condition is false
    template <typename Arg>
    void FieldIf(const bool condition, const std::string_view name, const Arg& arg)
    {
        if (condition)
        {
            Field(name, arg);
        }
    }

    // Overload where argument is also used as condition.
    // Useful for boilerplate where we want to write int field if it's not zero
    template <typename Arg>
    void FieldIf(const std::string_view name, const Arg& arg)
    {
        if constexpr (std::is_same_v<Arg, simulation::FixedPoint>)
        {
            FieldIf(arg != 0_fp, name, arg);
        }
        else
        {
            FieldIf(arg, name, arg);
        }
    }

    // The same as FieldIf but arg != Arg::kNone as condition
    template <typename Arg>
    void EnumFieldIfNotNone(const std::string_view name, const Arg& arg)
    {
        FieldIf(arg != Arg::kNone, name, arg);
    }

private:
    fmt::format_context& ctx;
    bool written_first_field_ = false;
};
}  // namespace simulation
