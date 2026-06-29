#include "Logging.hpp"

#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include <QStandardPaths>
#include <QString>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

#include "application/diagnostics/Logging.hpp"

namespace random_walker::infrastructure {

    namespace {

        std::mutex logging_mutex;
        std::filesystem::path active_file_path;
        bool logging_initialized = false;

        [[nodiscard]] std::filesystem::path path_from_qstring(
            const QString& path
        ) {
#ifdef _WIN32
            return std::filesystem::path(path.toStdWString());
#else
            return std::filesystem::path(path.toStdString());
#endif
        }

        [[nodiscard]] std::filesystem::path default_log_directory() {
            const QString location = QStandardPaths::writableLocation(
                QStandardPaths::AppLocalDataLocation
            );

            if (!location.isEmpty()) {
                return path_from_qstring(location) / "logs";
            }

            return std::filesystem::current_path() / "logs";
        }

        [[nodiscard]] spdlog::level::level_enum to_spdlog_level(
            LogLevel level
        ) noexcept {
            switch (level) {
            case LogLevel::Trace:
                return spdlog::level::trace;
            case LogLevel::Debug:
                return spdlog::level::debug;
            case LogLevel::Info:
                return spdlog::level::info;
            case LogLevel::Warning:
                return spdlog::level::warn;
            case LogLevel::Error:
                return spdlog::level::err;
            case LogLevel::Critical:
                return spdlog::level::critical;
            case LogLevel::Off:
                return spdlog::level::off;
            }

            return spdlog::level::info;
        }

        [[nodiscard]] spdlog::level::level_enum to_spdlog_level(
            application::LogLevel level
        ) noexcept {
            switch (level) {
            case application::LogLevel::Debug:
                return spdlog::level::debug;
            case application::LogLevel::Info:
                return spdlog::level::info;
            case application::LogLevel::Warning:
                return spdlog::level::warn;
            case application::LogLevel::Error:
                return spdlog::level::err;
            }

            return spdlog::level::info;
        }

        void write_log_message(
            application::LogLevel level
            , std::string_view category
            , std::string_view message
        ) noexcept {
            try {
                std::lock_guard lock(logging_mutex);
                if (!logging_initialized) {
                    return;
                }

                spdlog::log(
                    to_spdlog_level(level)
                    , "[{}] {}"
                    , std::string(category)
                    , std::string(message)
                );
            } catch (...) {
            }
        }
    }

    std::string logging_initialization_error_text(
        LoggingInitializationError error
    ) {
        switch (error) {
        case LoggingInitializationError::EmptyLogFileName:
            return "log file name is empty";
        case LoggingInitializationError::InvalidRotationPolicy:
            return "log rotation policy is invalid";
        case LoggingInitializationError::DirectoryCreationFailed:
            return "failed to create log directory";
        case LoggingInitializationError::SinkCreationFailed:
            return "failed to create rotating log file sink";
        }

        return "unknown logging initialization error";
    }

    LoggingConfig default_logging_config() {
        LoggingConfig config;
        config.log_directory = default_log_directory();
#ifndef NDEBUG
        config.level = LogLevel::Debug;
#endif
        return config;
    }

    std::optional<LoggingInitializationError> initialize_logging(
        const LoggingConfig& config
    ) noexcept {
        if (config.log_file_name.empty()) {
            return LoggingInitializationError::EmptyLogFileName;
        }
        if (config.max_file_size_bytes == 0U || config.max_files == 0U) {
            return LoggingInitializationError::InvalidRotationPolicy;
        }

        try {
            std::filesystem::create_directories(config.log_directory);
        } catch (...) {
            return LoggingInitializationError::DirectoryCreationFailed;
        }

        const std::filesystem::path log_file_path =
            config.log_directory / config.log_file_name;

        try {
            auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                log_file_path.string()
                , config.max_file_size_bytes
                , config.max_files
            );
            sink->set_level(to_spdlog_level(config.level));

            auto logger = std::make_shared<spdlog::logger>(
                "random-walker"
                , std::move(sink)
            );
            logger->set_level(to_spdlog_level(config.level));
            logger->set_pattern(
                "[%Y-%m-%d %H:%M:%S.%e] [%t] [%n] [%^%l%$] %v"
            );
            logger->flush_on(spdlog::level::warn);

            std::lock_guard lock(logging_mutex);
            spdlog::set_default_logger(std::move(logger));
            spdlog::flush_on(spdlog::level::warn);
            active_file_path = log_file_path;
            logging_initialized = true;
            application::set_log_sink(write_log_message);
        } catch (const spdlog::spdlog_ex&) {
            return LoggingInitializationError::SinkCreationFailed;
        } catch (const std::exception&) {
            return LoggingInitializationError::SinkCreationFailed;
        } catch (...) {
            return LoggingInitializationError::SinkCreationFailed;
        }

        return std::nullopt;
    }

    void shutdown_logging() noexcept {
        try {
            application::clear_log_sink();
            {
                std::lock_guard lock(logging_mutex);
                logging_initialized = false;
                active_file_path.clear();
            }
            spdlog::shutdown();
        } catch (...) {
        }
    }

    std::filesystem::path active_log_file_path() {
        std::lock_guard lock(logging_mutex);
        return active_file_path;
    }
}
