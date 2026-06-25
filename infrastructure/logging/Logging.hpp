#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace random_walker::infrastructure {
    enum class LogLevel {
        Trace
        , Debug
        , Info
        , Warning
        , Error
        , Critical
        , Off
    };

    enum class LoggingInitializationError {
        EmptyLogFileName
        , InvalidRotationPolicy
        , DirectoryCreationFailed
        , SinkCreationFailed
    };

    struct LoggingConfig {
        std::filesystem::path log_directory;
        std::string log_file_name = "random-walker.log";
        std::size_t max_file_size_bytes = 5U * 1024U * 1024U;
        std::size_t max_files = 5U;
        LogLevel level = LogLevel::Info;
    };

    [[nodiscard]] LoggingConfig default_logging_config();

    [[nodiscard]] std::optional<LoggingInitializationError>
    initialize_logging(
        const LoggingConfig& config = default_logging_config()
    ) noexcept;

    void shutdown_logging() noexcept;

    [[nodiscard]] std::filesystem::path active_log_file_path();

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
