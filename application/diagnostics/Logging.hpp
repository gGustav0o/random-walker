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

    namespace log_category {
        inline constexpr std::string_view application = "application";
        inline constexpr std::string_view auto_markers = "auto-markers";
        inline constexpr std::string_view bootstrap = "bootstrap";
        inline constexpr std::string_view diagnostics = "diagnostics";
        inline constexpr std::string_view settings = "settings";
        inline constexpr std::string_view viewmodel = "viewmodel";
    }

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
