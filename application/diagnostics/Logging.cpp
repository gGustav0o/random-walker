#include "Logging.hpp"

#include <cstdio>
#include <mutex>
#include <utility>

namespace random_walker::application {
    namespace {
        std::mutex log_sink_mutex;
        LogSink current_log_sink;

        void write_fallback_diagnostic(
            const char* message
        ) noexcept {
            std::fputs(message, stderr);
            std::fputc('\n', stderr);
        }

        void log_message(
            LogLevel level
            , std::string_view category
            , std::string_view message
        ) noexcept {
            try {
                LogSink sink;
                {
                    std::lock_guard lock(log_sink_mutex);
                    sink = current_log_sink;
                }

                if (sink) {
                    sink(level, category, message);
                }
            } catch (...) {
                write_fallback_diagnostic(
                    "Application log sink threw; message was dropped"
                );
            }
        }
    }

    void set_log_sink(LogSink sink) {
        std::lock_guard lock(log_sink_mutex);
        current_log_sink = std::move(sink);
    }

    void clear_log_sink() noexcept {
        try {
            std::lock_guard lock(log_sink_mutex);
            current_log_sink = {};
        } catch (...) {
            write_fallback_diagnostic(
                "Application log sink cleanup failed"
            );
        }
    }

    void log_debug(
        std::string_view category
        , std::string_view message
    ) noexcept {
        log_message(LogLevel::Debug, category, message);
    }

    void log_info(
        std::string_view category
        , std::string_view message
    ) noexcept {
        log_message(LogLevel::Info, category, message);
    }

    void log_warning(
        std::string_view category
        , std::string_view message
    ) noexcept {
        log_message(LogLevel::Warning, category, message);
    }

    void log_error(
        std::string_view category
        , std::string_view message
    ) noexcept {
        log_message(LogLevel::Error, category, message);
    }
}
