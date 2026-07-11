#include "Logger.h"

#include <print>
#include <source_location>
#include <string>

namespace
{
    std::string_view base_name(std::string_view path)
    {
        size_t pos = path.find_last_of("\\");
        if (pos == std::string_view::npos)
            return path;
        return path.substr(pos + 1);
    }
}

void log_message(LogLevel level, std::string const& message, std::source_location const& loc)
{
    switch (level)
    {
    case LogLevel::Debug:
        std::print("[DEBUG] ");
        break;
    case LogLevel::Info:
        std::print("[INFO ] ");
        break;
    case LogLevel::Warning:
        std::print("[WARN ] ");
        break;
    case LogLevel::Error:
        std::print("[ERROR] ");
        break;
    }

    std::println("{}:{} {}", base_name(loc.file_name()), loc.line(), message);
}
