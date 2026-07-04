#include <array>
#include <optional>
#include <string>

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest>

#include "application/diagnostics/Logging.hpp"
#include "infrastructure/logging/Logging.hpp"

namespace logging = random_walker::infrastructure;

class InfrastructureLoggingTests final : public QObject {
    Q_OBJECT

private slots:
    void cleanup();
    void initialization_error_text_is_available_for_each_error();
    void default_config_flushes_info_and_above();
    void flush_logging_is_noop_before_initialization();
    void flush_logging_flushes_initialized_logger();
    void rejects_empty_log_file_name();
    void rejects_zero_rotation_file_size();
    void rejects_zero_rotation_file_count();
};

void InfrastructureLoggingTests::cleanup() {
    logging::shutdown_logging();
}

void InfrastructureLoggingTests::initialization_error_text_is_available_for_each_error() {
    constexpr std::array errors {
        logging::LoggingInitializationError::EmptyLogFileName,
        logging::LoggingInitializationError::InvalidRotationPolicy,
        logging::LoggingInitializationError::DirectoryCreationFailed,
        logging::LoggingInitializationError::SinkCreationFailed
    };

    for (const logging::LoggingInitializationError error : errors) {
        const std::string text = logging::logging_initialization_error_text(
            error
        );

        QVERIFY(!text.empty());
    }
}

void InfrastructureLoggingTests::default_config_flushes_info_and_above() {
    const logging::LoggingConfig config = logging::default_logging_config();

    QCOMPARE(
        static_cast<int>(config.flush_level)
        , static_cast<int>(logging::LogLevel::Info)
    );
}

void InfrastructureLoggingTests::flush_logging_is_noop_before_initialization() {
    logging::flush_logging();

    QVERIFY(logging::active_log_file_path().empty());
}

void InfrastructureLoggingTests::flush_logging_flushes_initialized_logger() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    logging::LoggingConfig config;
    config.log_directory = directory.path().toStdString();
    config.log_file_name = "random-walker-test.log";
    config.flush_level = logging::LogLevel::Info;

    const std::optional<logging::LoggingInitializationError> error =
        logging::initialize_logging(config);
    QVERIFY(!error.has_value());

    random_walker::application::log_info(
        random_walker::application::log_category::diagnostics
        , "flush test message"
    );
    logging::flush_logging();

    const QFileInfo log_file(
        directory.filePath(QString::fromStdString(config.log_file_name))
    );
    QVERIFY(log_file.exists());
    QVERIFY(log_file.size() > 0);
}

void InfrastructureLoggingTests::rejects_empty_log_file_name() {
    logging::LoggingConfig config;
    config.log_file_name.clear();

    const std::optional<logging::LoggingInitializationError> error =
        logging::initialize_logging(config);

    QVERIFY(error.has_value());
    QCOMPARE(
        static_cast<int>(*error)
        , static_cast<int>(
            logging::LoggingInitializationError::EmptyLogFileName
        )
    );
    QVERIFY(logging::active_log_file_path().empty());
}

void InfrastructureLoggingTests::rejects_zero_rotation_file_size() {
    logging::LoggingConfig config;
    config.log_file_name = "random-walker-test.log";
    config.max_file_size_bytes = 0U;

    const std::optional<logging::LoggingInitializationError> error =
        logging::initialize_logging(config);

    QVERIFY(error.has_value());
    QCOMPARE(
        static_cast<int>(*error)
        , static_cast<int>(
            logging::LoggingInitializationError::InvalidRotationPolicy
        )
    );
    QVERIFY(logging::active_log_file_path().empty());
}

void InfrastructureLoggingTests::rejects_zero_rotation_file_count() {
    logging::LoggingConfig config;
    config.log_file_name = "random-walker-test.log";
    config.max_files = 0U;

    const std::optional<logging::LoggingInitializationError> error =
        logging::initialize_logging(config);

    QVERIFY(error.has_value());
    QCOMPARE(
        static_cast<int>(*error)
        , static_cast<int>(
            logging::LoggingInitializationError::InvalidRotationPolicy
        )
    );
    QVERIFY(logging::active_log_file_path().empty());
}

QTEST_GUILESS_MAIN(InfrastructureLoggingTests)
#include "LoggingTests.moc"
