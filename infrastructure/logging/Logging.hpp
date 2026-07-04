#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>

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
        // Single application-wide minimum level. Per-category and per-sink
        // overrides are intentionally not modeled yet.
        LogLevel level = LogLevel::Info;
        LogLevel flush_level = LogLevel::Info;
    };

    [[nodiscard]] std::string
    logging_initialization_error_text(LoggingInitializationError error);

    [[nodiscard]] LoggingConfig default_logging_config();

    [[nodiscard]] std::optional<LoggingInitializationError>
    initialize_logging(
        const LoggingConfig& config = default_logging_config()
    ) noexcept;

    void flush_logging() noexcept;
    void shutdown_logging() noexcept;

    [[nodiscard]] std::filesystem::path active_log_file_path();
}
