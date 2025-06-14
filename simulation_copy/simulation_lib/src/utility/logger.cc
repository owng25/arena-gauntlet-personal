#include "utility/logger.h"

#include <mutex>

#include "spdlog/async.h"
#include "spdlog/cfg/helpers-inl.h"
#include "spdlog/details/null_mutex.h"
#include "spdlog/sinks/ansicolor_sink.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/spdlog.h"

// We can't have any references to spdlog in headers due to Unreal header conflict,
// so this file handles wrapping some of the functionality
// Available sinks https://spdlog.docsforge.com/v1.x/4.sinks/#available-sinks

namespace simulation
{

template <typename Mutex>
class LoggerSink : public spdlog::sinks::base_sink<Mutex>
{
public:
    LoggerSink(LoggerHandlerCallback callback)
    {
        callback_ = callback;
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        // Format message
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        std::string output = fmt::to_string(formatted);

        // Trim newlines
        spdlog::cfg::helpers::trim_(output);

        // Send to the callback
        callback_(static_cast<LogLevel>(msg.level), output);
    }

    void flush_() override
    {
        // Nothing to do here, but must override
    }

private:
    LoggerHandlerCallback callback_;
};

using LoggerSink_mt = LoggerSink<std::mutex>;
using LoggerSink_st = LoggerSink<spdlog::details::null_mutex>;

Logger::Logger(const bool enable_debug_logs, const std::string& default_category_name)
    : enable_debug_logs_(enable_debug_logs),
      default_category_name_(default_category_name)
{
    // Create default logger
    GetOrCreateCategory(default_category_name_);
}

Logger::~Logger() {}

Logger::InternalCategory& Logger::GetOrCreateCategory(const std::string_view category_name)
{
    // Already exists
    if (HasCategory(category_name))
    {
        return categories_.at(category_name);
    }

    // Create
    // NOTE: We don't register with spdlog::register_logger because we don't want to store any
    // global state.
    // See:
    // https://spdlog.docsforge.com/v1.x/2.creating-loggers/#creating-loggers-using-the-factory-functions
    auto logger = std::make_shared<spdlog::logger>(std::string{category_name}, std::begin(sinks_), std::end(sinks_));

    // Log everything
    // https://spdlog.docsforge.com/v1.x/api/spdlog/level/level_enum/
    logger->set_level(enable_debug_logs_ ? spdlog::level::trace : spdlog::level::info);

    // Set custom pattern if set
    if (!custom_log_pattern_.empty())
    {
        logger->set_pattern(custom_log_pattern_);
    }

    InternalCategory category;
    category.is_enabled = true;
    category.logger = logger;

    // Internally keep track of it
    categories_[category_name] = category;

    return categories_.at(category_name);
}

void Logger::EnableCategory(const std::string_view category_name)
{
    GetOrCreateCategory(category_name).is_enabled = true;
}

void Logger::DisableCategory(const std::string_view category_name)
{
    GetOrCreateCategory(category_name).is_enabled = false;
}

void Logger::UpdateCategoriesSinks()
{
    for (const auto& [category_name, category] : categories_)
    {
        category.logger->sinks() = sinks_;
    }
}

Logger& Logger::SinkAddCustom(const LoggerHandlerCallback& log_handler)
{
    // Update the sink
    AddSink(std::make_shared<LoggerSink_st>(log_handler));
    return *this;
}

Logger& Logger::SinkAddStdout(const bool with_color)
{
    if (with_color)
    {
        AddSink(std::make_shared<spdlog::sinks::stdout_color_sink_st>());
    }
    else
    {
        AddSink(std::make_shared<spdlog::sinks::stdout_sink_st>());
    }

    return *this;
}

Logger& Logger::SinkAddFile(const std::string_view path)
{
    AddSink(std::make_shared<spdlog::sinks::basic_file_sink_st>(path.data()));
    return *this;
}

void Logger::AddSink(std::shared_ptr<spdlog::sinks::sink> sink)
{
    sinks_.push_back(sink);
    UpdateCategoriesSinks();
}

void Logger::SinksClear()
{
    sinks_.clear();
    UpdateCategoriesSinks();
}

void Logger::SetLogsPattern(const std::string_view& pattern)
{
    custom_log_pattern_ = std::string(pattern);

    // Update existing
    for (auto& [category_name, logger_category] : categories_)
    {
        logger_category.logger->set_pattern(custom_log_pattern_);
    }
}

void Logger::Log(const std::string_view category_name, const LogLevel log_level, const std::string& str)
{
    if (!is_enabled_)
    {
        // Global logging is disabled
        return;
    }

    const auto& category = GetOrCreateCategory(category_name);
    if (!category.is_enabled)
    {
        // This logger category is disabled
        return;
    }

    // Something went wrong
    if (!category.logger)
    {
        assert(false);
        return;
    }

    // Get the appropriate log level for spdlog
    const auto spd_level = static_cast<spdlog::level::level_enum>(log_level);

    // Send to spdlog
    category.logger->log(spd_level, str);
}

}  // namespace simulation
