#include "version.h"

#include "version_git_internal.h"

namespace simulation
{
// Gets the current version of the simulation
static constexpr std::string_view GetVersionStringView()
{
    return kGitSha1Short;
}
std::string GetVersion()
{
    return std::string{GetVersionStringView()};
}
}  // namespace simulation
