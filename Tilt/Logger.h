#pragma once

#include <source_location>
#include <string>

enum class LogLevel
{
    Debug,
    Info,
    Warning,
    Error
};

#ifdef NDEBUG
#define TILT_LOG_DEBUG(msg) ((void)0)
#else
#define TILT_LOG_DEBUG(msg) log_message(LogLevel::Debug, msg, std::source_location::current())
#endif

#define TILT_LOG_INFO(msg) log_message(LogLevel::Info, msg, std::source_location::current())
#define TILT_LOG_WARN(msg) log_message(LogLevel::Warning, msg, std::source_location::current())
#define TILT_LOG_ERROR(msg) log_message(LogLevel::Error, msg, std::source_location::current())

void log_message(LogLevel level, std::string const& message, std::source_location const& loc);
