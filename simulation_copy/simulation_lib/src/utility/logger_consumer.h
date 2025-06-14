#pragma once

#include <string>

#include "data/constants.h"
#include "data/enums.h"
#include "utility/logger.h"

namespace simulation
{

/* -------------------------------------------------------------------------------------------------------
 * LoggerConsumer
 *
 * Interface for the Logger consumers
 * --------------------------------------------------------------------------------------------------------
 */
class LoggerConsumer
{
public:
    // Default destructor
    virtual ~LoggerConsumer() {}

    // Returns the logger the world uses
    virtual std::shared_ptr<Logger> GetLogger() const = 0;

    // Builds a nice log prefix for the specified entity
    virtual void BuildLogPrefixFor(const EntityID id, std::string* out_string) const = 0;

    // Gets the current time step count
    virtual int GetTimeStepCount() const = 0;

    // The logger category name for this system logs
    virtual std::string_view GetLoggerCategoryName() const
    {
        return GetLogger()->GetDefaultCategoryName();
    }

    // Helper function to get the string format with the time step counter
    std::string GetLoggerFormatWithTimeStepCounter(const std::string_view fmt) const
    {
        return fmt::format("[time step = {}] {}", GetTimeStepCount(), fmt);
    }

    // Helper functions to get string format with the entity
    std::string GetLoggerFormatWithEntity(const EntityID id, const std::string_view fmt) const
    {
        std::string result;
        BuildLogPrefixFor(id, &result);
        fmt::format_to(std::back_inserter(result), " {}", fmt);
        return result;
    }

    // Log with:
    // - user defined category_name
    // - time step counter prefixed
    template <typename... Args>
    void Log(
        const std::string_view category_name,
        const LogLevel logger_level,
        const fmt::format_string<Args...>& fmt,
        Args&&... args) const
    {
        std::shared_ptr<Logger> logger = GetLogger();
        if (logger_level != LogLevel::kDebug || logger->AreDebugLogsEnabled())
        {
            const auto fmt_view = fmt.get();
            const std::string str_fmt =
                GetLoggerFormatWithTimeStepCounter(std::string_view(fmt_view.begin(), fmt_view.size()));
            logger->Log(category_name, logger_level, fmt::runtime(str_fmt), std::forward<Args>(args)...);
        }
    }

    // Log with:
    // - entity prefix
    // - user defined category_name
    // - time step counter prefixed
    template <typename... Args>
    void Log(
        const EntityID id,
        const std::string_view category_name,
        const LogLevel logger_level,
        const fmt::format_string<Args...>& fmt,
        Args&&... args) const
    {
        std::shared_ptr<Logger> logger = GetLogger();
        if (logger_level != LogLevel::kDebug || logger->AreDebugLogsEnabled())
        {
            auto basic_view = fmt.get();
            const std::string str_fmt =
                GetLoggerFormatWithEntity(id, std::string_view(basic_view.begin(), basic_view.size()));
            Log(category_name, logger_level, fmt::runtime(str_fmt), std::forward<Args>(args)...);
        }
    }

    // Log with:
    // - time step counter prefixed
    template <typename... Args>
    void Log(const LogLevel logger_level, const fmt::format_string<Args...>& fmt, Args&&... args) const
    {
        Log(GetLoggerCategoryName(), logger_level, fmt, std::forward<Args>(args)...);
    }

    bool AreDebugLogsEnabled() const
    {
        return GetLogger()->AreDebugLogsEnabled();
    }

    // Log with:
    // - entity prefix
    // - time step counter prefixed
    template <typename... Args>
    void Log(const EntityID id, const LogLevel logger_level, const fmt::format_string<Args...>& fmt, Args&&... args)
        const
    {
        Log(id, GetLoggerCategoryName(), logger_level, fmt, std::forward<Args>(args)...);
    }

    // Log with:
    // - time step counter prefixed
    template <typename... Args>
    void LogTrace(const fmt::format_string<Args...>& fmt, Args&&... args) const
    {
        Log(LogLevel::kTrace, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void LogDebug(const fmt::format_string<Args...>& fmt, Args&&... args) const
    {
        Log(LogLevel::kDebug, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void LogInfo(const fmt::format_string<Args...>& fmt, Args&&... args) const
    {
        Log(LogLevel::kInfo, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void LogWarn(const fmt::format_string<Args...>& fmt, Args&&... args) const
    {
        Log(LogLevel::kWarn, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void LogErr(const fmt::format_string<Args...>& fmt, Args&&... args) const
    {
        Log(LogLevel::kErr, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void LogCritical(const fmt::format_string<Args...>& fmt, Args&&... args) const
    {
        Log(LogLevel::kCritical, fmt, std::forward<Args>(args)...);
    }

    // Log with:
    // - entity prefix
    // - time step counter prefixed
    template <typename... Args>
    void LogTrace(const EntityID id, const fmt::format_string<Args...>& fmt, Args&&... args) const
    {
        Log(id, LogLevel::kTrace, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void LogDebug(const EntityID id, const fmt::format_string<Args...>& fmt, Args&&... args) const
    {
        Log(id, LogLevel::kDebug, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void LogInfo(const EntityID id, const fmt::format_string<Args...>& fmt, Args&&... args) const
    {
        Log(id, LogLevel::kInfo, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void LogWarn(const EntityID id, const fmt::format_string<Args...>& fmt, Args&&... args) const
    {
        Log(id, LogLevel::kWarn, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void LogErr(const EntityID id, const fmt::format_string<Args...>& fmt, Args&&... args) const
    {
        Log(id, LogLevel::kErr, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void LogCritical(const EntityID id, const fmt::format_string<Args...>& fmt, Args&&... args) const
    {
        Log(id, LogLevel::kCritical, fmt, std::forward<Args>(args)...);
    }
};

}  // namespace simulation
