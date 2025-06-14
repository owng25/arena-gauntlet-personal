#pragma once

#include "data/containers/game_data_container.h"
#include "gtest/gtest.h"

class TestEnvironment : public ::testing::Environment
{
public:
    explicit TestEnvironment(const bool enable_logger)
    {
        enable_logger_ = enable_logger;
    }
    ~TestEnvironment() override = default;

    static std::shared_ptr<simulation::Logger> GetLogger()
    {
        static LoggerContainer logger_container{enable_logger_};
        return logger_container.logger_;
    }

    template <typename T>
    static std::shared_ptr<T> DefaultContainer()
    {
        static auto ptr = std::make_shared<T>();
        return ptr;
    }

    static std::shared_ptr<simulation::GameDataContainer> DefaultGameData()
    {
        static auto ptr = std::make_shared<simulation::GameDataContainer>(GetLogger());
        return ptr;
    }

private:
    // Wrapper class around container to manage lifecycle of the logger
    class LoggerContainer
    {
    public:
        // Create nodes
        explicit LoggerContainer(const bool is_enabled)
        {
            static constexpr bool enable_debug_logs = true;
            logger_ = simulation::Logger::Create(enable_debug_logs);
            logger_->SinkAddStdout();
            // logger_->SetLogsPattern("[%Y-%m-%d %T.%e] [%=15!n] [%l] %v");
            // logger_->DisableCategory(simulation::LogCategory::kEffect);
            // logger_->DisableCategory(simulation::LogCategory::kMovement);
            // logger_->DisableCategory(simulation::LogCategory::kAbility);
            // logger_->DisableCategory(simulation::LogCategory::kDecision);
            // logger_->DisableCategory(simulation::LogCategory::kAttachedEffect);

            if (is_enabled)
            {
                logger_->Enable();
            }
            else
            {
                logger_->Disable();
            }
            // logger_->FlushEverySeconds(10);
        }

        std::shared_ptr<simulation::Logger> logger_ = nullptr;
    };

    // Is the logger enabled?
    static bool enable_logger_;
};
