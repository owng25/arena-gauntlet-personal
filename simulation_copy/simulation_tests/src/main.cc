#ifdef _MSC_VER
#pragma warning(push)            // Save warning settings.
#pragma warning(disable : 4996)  // Disable deprecated warning for std::checked_array
#endif

#include <string_view>

#include "fmt/core.h"
#include "gtest/gtest.h"
#include "profiling/illuvium_profiling.h"
#include "spdlog/spdlog.h"
#include "test_environment.h"
#include "utility/file_helper.h"
#include "utility/version.h"

TEST(TestFramework, Test)
{
    ASSERT_EQ(42, 42);
}

int main(int argc, char** argv)
{
    simulation::FileHelper::SetExecutableDirectory(std::filesystem::path(argv[0]).parent_path());  // NOLINT

    fmt::println("Running main() from Tests/main.cc, version = %s\n", simulation::GetVersion().c_str());
    ::testing::InitGoogleTest(&argc, argv);

#ifdef ENABLE_PROFILING
    // Start profiling and listening on port 28077
    EASY_PROFILER_ENABLE;
    EASY_MAIN_THREAD;
    profiler::startListen();
#endif

    // Read custom command line arguments
    bool enable_logger = true;
    for (int i = 1; i < argc; i++)
    {
        if (std::string_view(argv[i]) == "--logger-disable")  // NOLINT
        {
            enable_logger = false;
        }
    }

    ::testing::AddGlobalTestEnvironment(new TestEnvironment{enable_logger});
    const int status_code = RUN_ALL_TESTS();

    // shutdown the logger
    spdlog::shutdown();

#ifdef ENABLE_PROFILING
    // Stop profiling
    profiler::stopListen();

    // Save a copy of results to file
    profiler::dumpBlocksToFile("simulation.prof");
#endif

    return status_code;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
