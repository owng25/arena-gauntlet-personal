#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "data/enums.h"
#include "data/logger_enums.h"

// forward declare the spd logger
namespace spdlog
{
class logger;

namespace sinks
{
class sink;
}
}  // namespace spdlog

namespace simulation
{
// Custom type for the logger handler
typedef std::function<void(const LogLevel log_level, const std::string_view)> LoggerHandlerCallback;

/* -------------------------------------------------------------------------------------------------------
 * Logger
 *
 * This class defines the logger we use inside the library
 * --------------------------------------------------------------------------------------------------------
 */
class Logger final
{
public:
    static constexpr std::string_view kDefaultLogPattern = "[%Y-%m-%d %T.%e] [%=15!n] [%l] %v";

    // Create a new instance
    static std::shared_ptr<Logger> Create(
        const bool enable_debug_logs = true,
        const std::string& default_category_name = std::string{LogCategory::kDefault})
    {
        // NOTE: can't access private constructor with make_shared
        return std::shared_ptr<Logger>(new Logger(enable_debug_logs, default_category_name));
    }

    // Not copyable or movable
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    ~Logger();

    // Adds a new custom log handler
    Logger& SinkAddCustom(const LoggerHandlerCallback& log_handler);

    // Log handle to stdout
    Logger& SinkAddStdout(const bool with_color = true);

    // Log to file
    Logger& SinkAddFile(const std::string_view path);

    // Clear all the sinks
    void SinksClear();

    // Sets the custom formatting pattern
    // https://spdlog.docsforge.com/v1.x/3.custom-formatting/
    void SetLogsPattern(const std::string_view& pattern);

    // Enables all the loggers
    void Enable()
    {
        is_enabled_ = true;
    }

    // Disable all the loggers
    void Disable()
    {
        is_enabled_ = false;
    }

    // Gets the default category name
    std::string_view GetDefaultCategoryName() const
    {
        return default_category_name_;
    }

    // Enables this specific category name
    void EnableCategory(const std::string_view category_name);

    // Disables this specific category name
    void DisableCategory(const std::string_view category_name);

    // Does the category_name exist?
    bool HasCategory(const std::string_view category_name) const
    {
        return categories_.count(category_name) > 0;
    }

    bool AreDebugLogsEnabled() const
    {
        return enable_debug_logs_;
    }

    // Log a string with the category_name and level
    void Log(const std::string_view category_name, const LogLevel log_level, const std::string& str);

    // Log with the default category
    template <typename... Args>
    void Log(const LogLevel log_level, const fmt::format_string<Args...> fmt, Args&&... args)
    {
        if (log_level != LogLevel::kDebug || AreDebugLogsEnabled())
        {
            Log(default_category_name_, log_level, fmt::format(fmt, std::forward<Args>(args)...));
        }
    }

    // Log with the default category
    template <typename... Args>
    void LogErr(const fmt::format_string<Args...>& fmt, Args&&... args)
    {
        Log(LogLevel::kErr, fmt, std::forward<Args>(args)...);
    }

    // Log with the default category
    template <typename... Args>
    void LogDebug(const fmt::format_string<Args...>& fmt, Args&&... args)
    {
        Log(LogLevel::kDebug, fmt, std::forward<Args>(args)...);
    }

    // Log with the default category
    template <typename... Args>
    void LogInfo(const fmt::format_string<Args...>& fmt, Args&&... args)
    {
        Log(LogLevel::kInfo, fmt, std::forward<Args>(args)...);
    }

    // Log with the default category
    template <typename... Args>
    void LogWarn(const fmt::format_string<Args...>& fmt, Args&&... args)
    {
        Log(LogLevel::kWarn, fmt, std::forward<Args>(args)...);
    }

    // Log with the custom category
    template <typename... Args>
    void Log(
        const std::string_view category_name,
        const LogLevel log_level,
        const fmt::format_string<Args...>& fmt,
        Args&&... args)
    {
        if (log_level != LogLevel::kDebug || AreDebugLogsEnabled())
        {
            Log(category_name, log_level, fmt::format(fmt, std::forward<Args>(args)...));
        }
    }

private:
    // Used internally to represent a category
    struct InternalCategory
    {
        bool is_enabled = true;
        std::shared_ptr<spdlog::logger> logger;
    };

    Logger(const bool enable_debug_logs, const std::string& default_category_name);

    // Get or Creates and empty logger with that category name
    Logger::InternalCategory& GetOrCreateCategory(const std::string_view category_name);

    // Adds a new sink
    void AddSink(std::shared_ptr<spdlog::sinks::sink> sink);

    // Updates the sinks for all the categories_
    void UpdateCategoriesSinks();

    // Are debug logs enabled?
    bool enable_debug_logs_ = true;

    // The default category name to use for the logs
    std::string default_category_name_;

    // The custom log pattern used
    // https://spdlog.docsforge.com/v1.x/3.custom-formatting/
    std::string custom_log_pattern_ = std::string(kDefaultLogPattern);

    // Runtime flag to disable/enable logging
    bool is_enabled_ = true;

    // Keep track of all the logger categories
    // Key: The category name
    // Value: The actual spd log
    std::unordered_map<std::string_view, InternalCategory> categories_;

    // Keep track of all the sink we use for the loggers
    std::vector<std::shared_ptr<spdlog::sinks::sink>> sinks_;
};

}  // namespace simulation
