#pragma once

#include <string>
#include <vector>

#include "utility/fmtlib.h"

namespace simulation
{
#define ILLUVIUM_FUNCTION_NAME __FUNCTION__

/* -------------------------------------------------------------------------------------------------------
 * String
 *
 * This defines string helper functions
 * --------------------------------------------------------------------------------------------------------
 */
class String
{
public:
    // concatenates two std::string_view objects in to std::string
    // used as replacement for std::string::operator+
    static std::string Concat(std::string_view a, std::string_view b)
    {
        return fmt::format("{}{}", a, b);
    }

    // Convert the string to lower case in place
    static void ToLower(std::string& str);

    // Convert the string to lower case and return the result as a copy
    static std::string ToLowerCopy(const std::string_view str);

    // Trim from end of string (right)
    static std::string& RightTrim(std::string& s, const char* t)
    {
        s.erase(s.find_last_not_of(t) + 1);
        return s;
    }

    // Trim from beginning of string (left)
    static std::string& LeftTrim(std::string& s, const char* t)
    {
        s.erase(0, s.find_first_not_of(t));
        return s;
    }

    // Trim from both ends of string (right then left)
    static std::string& Trim(std::string& s, const char* t)
    {
        return LeftTrim(RightTrim(s, t), t);
    }

    static constexpr bool StartsWith(std::string_view s, std::string_view prefix)
    {
        return s.substr(0, (std::min)(s.size(), prefix.size())) == prefix;
    }

    static constexpr bool EndsWith(std::string_view s, std::string_view suffix)
    {
        if (suffix.size() > s.size())
        {
            return false;
        }

        return s.substr(s.size() - suffix.size(), suffix.size()) == suffix;
    }

    template <typename... FmtArgs>
    static void FormatTo(std::string& s, fmt::format_string<FmtArgs...> format, FmtArgs&&... fmt_args)
    {
        fmt::format_to(std::back_inserter(s), format, std::forward<FmtArgs>(fmt_args)...);
    }

    template <typename... FmtArgs>
    static void AppendLineAndFormat(std::string& s, fmt::format_string<FmtArgs...> format, FmtArgs&&... fmt_args)
    {
        if (!s.empty())
        {
            s.push_back('\n');
        }
        FormatTo(s, format, std::forward<FmtArgs>(fmt_args)...);
    }

    // Joins vector elements into a string with separator
    template <typename TValueVector>
    static std::string JoinBy(const std::vector<TValueVector>& vector, std::string_view separator)
    {
        std::string result;
        for (size_t i = 0; i < vector.size(); ++i)
        {
            FormatTo(result, "{}", vector[i]);
            if (i < vector.size() - 1)
            {
                result += separator;
            }
        }
        return result;
    }
};

}  // namespace simulation
