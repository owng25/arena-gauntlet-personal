#pragma once

#include <string_view>

// clang-format off
#if defined(__clang__)
    #define ILLUVIUM_BEGIN_IGNORING_PROFILER_WARNINGS()                             \
        _Pragma("clang diagnostic push")                                            \
        _Pragma("clang diagnostic ignored \"-Wgnu-zero-variadic-macro-arguments\"")
    #define ILLUVIUM_END_IGNORING_PROFILER_WARNINGS()                               \
        _Pragma("clang diagnostic pop")
#else
    #define ILLUVIUM_BEGIN_IGNORING_PROFILER_WARNINGS()
    #define ILLUVIUM_END_IGNORING_PROFILER_WARNINGS()
#endif
// clang-format on

#ifdef ENABLE_PROFILING
#include <easy/profiler.h>

#define ILLUVIUM_PROFILE_FUNCTION(...)          \
    ILLUVIUM_BEGIN_IGNORING_PROFILER_WARNINGS() \
    EASY_FUNCTION(__VA_ARGS__)                  \
    ILLUVIUM_END_IGNORING_PROFILER_WARNINGS()

#define ILLUVIUM_PROFILE_BLOCK(...)             \
    ILLUVIUM_BEGIN_IGNORING_PROFILER_WARNINGS() \
    EASY_BLOCK(__VA_ARGS__)                     \
    ILLUVIUM_END_IGNORING_PROFILER_WARNINGS()

namespace simulation
{
inline void IlluviumStartProfiling()
{
    profiler::startCapture();
    EASY_MAIN_THREAD;
    profiler::startListen();
}

inline void IlluviumStopProfiling(const std::string_view dump_file)
{
    profiler::stopListen();

    // Dump to a file, if specified
    if (!dump_file.empty())
    {
        profiler::dumpBlocksToFile(dump_file.data());
    }
}
}  // namespace simulation

#else
#define ILLUVIUM_PROFILE_FUNCTION(...)
#define ILLUVIUM_PROFILE_BLOCK(...)
namespace simulation
{
inline void IlluviumStartProfiling() {}
inline void IlluviumStopProfiling(const std::string_view) {}
}  // namespace simulation
#endif
