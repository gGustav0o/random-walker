#pragma once

#include <functional>
#include <string_view>

namespace random_walker::application {
    enum class LogLevel {
        Debug
        , Info
        , Warning
        , Error
    };

    using LogSink = std::function<void(
        LogLevel
        , std::string_view
        , std::string_view
    )>;

    void set_log_sink(LogSink sink);
    void clear_log_sink() noexcept;

    void log_debug(
        std::string_view category
        , std::string_view message
    ) noexcept;
    void log_info(
        std::string_view category
        , std::string_view message
    ) noexcept;
    void log_warning(
        std::string_view category
        , std::string_view message
    ) noexcept;
    void log_error(
        std::string_view category
        , std::string_view message
    ) noexcept;
}
