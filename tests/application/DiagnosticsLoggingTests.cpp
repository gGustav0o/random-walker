#include <stdexcept>
#include <string>
#include <vector>

#include <QString>
#include <QtTest>

#include "application/diagnostics/Logging.hpp"

namespace diagnostics = random_walker::application;

namespace {
    struct CapturedLogMessage {
        diagnostics::LogLevel level = diagnostics::LogLevel::Info;
        std::string category;
        std::string message;
    };
}

class DiagnosticsLoggingTests final : public QObject {
    Q_OBJECT

private slots:
    void cleanup();
    void logging_without_sink_is_noop();
    void installed_sink_receives_log_message();
    void clear_log_sink_disables_delivery();
    void sink_exceptions_do_not_escape_logging_api();
};

void DiagnosticsLoggingTests::cleanup() {
    diagnostics::clear_log_sink();
}

void DiagnosticsLoggingTests::logging_without_sink_is_noop() {
    diagnostics::clear_log_sink();

    diagnostics::log_debug("diagnostics", "debug message");
    diagnostics::log_info("diagnostics", "info message");
    diagnostics::log_warning("diagnostics", "warning message");
    diagnostics::log_error("diagnostics", "error message");

    QVERIFY(true);
}

void DiagnosticsLoggingTests::installed_sink_receives_log_message() {
    std::vector<CapturedLogMessage> messages;
    diagnostics::set_log_sink(
        [&messages](
            diagnostics::LogLevel level
            , std::string_view category
            , std::string_view message
        ) {
            messages.push_back({
                .level = level
                , .category = std::string(category)
                , .message = std::string(message)
            });
        }
    );

    diagnostics::log_warning("settings", "repair required");

    QCOMPARE(static_cast<int>(messages.size()), 1);
    QCOMPARE(
        static_cast<int>(messages.front().level)
        , static_cast<int>(diagnostics::LogLevel::Warning)
    );
    QCOMPARE(QString::fromStdString(messages.front().category), QStringLiteral("settings"));
    QCOMPARE(QString::fromStdString(messages.front().message), QStringLiteral("repair required"));
}

void DiagnosticsLoggingTests::clear_log_sink_disables_delivery() {
    std::vector<CapturedLogMessage> messages;
    diagnostics::set_log_sink(
        [&messages](
            diagnostics::LogLevel level
            , std::string_view category
            , std::string_view message
        ) {
            messages.push_back({
                .level = level
                , .category = std::string(category)
                , .message = std::string(message)
            });
        }
    );

    diagnostics::clear_log_sink();
    diagnostics::log_info("application", "startup");

    QCOMPARE(static_cast<int>(messages.size()), 0);
}

void DiagnosticsLoggingTests::sink_exceptions_do_not_escape_logging_api() {
    diagnostics::set_log_sink(
        [](diagnostics::LogLevel, std::string_view, std::string_view) {
            throw std::runtime_error("sink failure");
        }
    );

    diagnostics::log_error("application", "failed");

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(DiagnosticsLoggingTests)
#include "DiagnosticsLoggingTests.moc"