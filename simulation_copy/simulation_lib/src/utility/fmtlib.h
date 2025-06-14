#pragma once

// Do not interfere if included in unreal.
// See https://github.com/fmtlib/fmt/pull/3200
#pragma push_macro("check")
#undef check

#ifdef _MSC_VER
#pragma warning(push)            // Save warning settings.
#pragma warning(disable : 4996)  // Disable deprecated warning for std::checked_array
#endif

#include "fmt/format.h"
#include "fmt/ranges.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#pragma pop_macro("check")
