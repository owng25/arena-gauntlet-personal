#include "utility/string.h"

#include <cctype>
#include <string>

namespace simulation
{
void String::ToLower(std::string& str)
{
    for (auto& c : str)
    {
        c = static_cast<char>(std::tolower(c));
    }
}

std::string String::ToLowerCopy(const std::string_view str)
{
    std::string lower{str};
    ToLower(lower);
    return lower;
}

}  // namespace simulation
