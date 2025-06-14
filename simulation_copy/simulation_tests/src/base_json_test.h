#pragma once

#include "base_test_fixtures.h"
#include "test_data_loader.h"

namespace simulation
{
class BaseJSONTest : public BaseTest
{
public:
    using Super = BaseTest;

public:
    void PostInitWorld() override
    {
        Super::PostInitWorld();

        loader = std::make_unique<TestingDataLoader>(world->GetLogger());

        auto devnull_logger = Logger::Create();
        devnull_logger->Disable();
        silent_loader = std::make_unique<TestingDataLoader>(devnull_logger);
    }

    void SetUp() override
    {
        Super::SetUp();
    }

protected:
    std::unique_ptr<TestingDataLoader> loader;
    std::unique_ptr<TestingDataLoader> silent_loader;
};

}  // namespace simulation
