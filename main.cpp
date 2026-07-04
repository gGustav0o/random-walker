#include <cstdlib>
#include <exception>
#include <memory>
#include <string>

#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QString>
#include <QUrl>

#include "bootstrap/AppContext.hpp"
#include "application/diagnostics/Logging.hpp"
#include "infrastructure/logging/Logging.hpp"

namespace {
    constexpr int kExitSuccess = 0;
    constexpr int kExitQmlLoadFailure = -1;

    class LoggingShutdownGuard final {
    public:
        ~LoggingShutdownGuard() {
            random_walker::infrastructure::shutdown_logging();
        }
    };
    std::terminate_handler previous_terminate_handler = nullptr;

    [[noreturn]] void handle_terminate() noexcept {
        random_walker::application::log_error(
            random_walker::application::log_category::application
            , "Application terminated unexpectedly"
        );
        random_walker::infrastructure::flush_logging();

        if (previous_terminate_handler != nullptr
            && previous_terminate_handler != handle_terminate) {
            previous_terminate_handler();
        }

        std::abort();
    }

    class TerminateGuard final {
    public:
        TerminateGuard() noexcept
            : previous_handler_(std::set_terminate(handle_terminate)) {
            previous_terminate_handler = previous_handler_;
        }

        ~TerminateGuard() {
            std::set_terminate(previous_handler_);
            previous_terminate_handler = nullptr;
        }

        TerminateGuard(const TerminateGuard&) = delete;
        TerminateGuard& operator=(const TerminateGuard&) = delete;

    private:
        std::terminate_handler previous_handler_ = nullptr;
    };

    void setup_high_dpi() {
#if defined(Q_OS_WIN) && QT_VERSION_CHECK(5, 6, 0) <= QT_VERSION && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    }
}

int main(int argc, char* argv[]) {
    setup_high_dpi();
    QGuiApplication app(argc, argv);

    const auto logging_error =
        random_walker::infrastructure::initialize_logging();

    const LoggingShutdownGuard logging_shutdown_guard;
    const TerminateGuard terminate_guard;
    if (logging_error.has_value()) {
        qWarning().noquote()
            << QStringLiteral("Failed to initialize logging:")
            << QString::fromStdString(
                random_walker::infrastructure::
                    logging_initialization_error_text(*logging_error)
            );
    } else {
        random_walker::application::log_info(
            random_walker::application::log_category::application
            , std::string("Logging initialized at ")
                + random_walker::infrastructure::active_log_file_path().string()
        );
        random_walker::application::log_info(
            random_walker::application::log_category::application
            , "Application startup"
        );
        random_walker::infrastructure::flush_logging();
    }

    int exit_code = kExitSuccess;
    {
        // The composition root must outlive the QML object tree because QML
        // bindings hold a non-owning reference to its ViewModel.
        std::unique_ptr<AppContext> context;
        QQmlApplicationEngine engine;
        context = std::make_unique<AppContext>(engine);

        engine.load(QUrl(QStringLiteral(
            "qrc:/qt/qml/random-walker/view/views/MainView.qml"
        )));

        if (engine.rootObjects().isEmpty()) {
            random_walker::application::log_error(
                random_walker::application::log_category::application
                , "QML root object creation failed"
            );
            random_walker::infrastructure::flush_logging();
            exit_code = kExitQmlLoadFailure;
        } else {
            random_walker::application::log_info(
                random_walker::application::log_category::application
                , "QML root object loaded"
            );
            random_walker::infrastructure::flush_logging();
            exit_code = app.exec();
            random_walker::application::log_info(
                random_walker::application::log_category::application
                , "Application event loop finished"
            );
            random_walker::infrastructure::flush_logging();
        }
    }

    random_walker::application::log_info(
        random_walker::application::log_category::application
        , "Application shutdown"
    );
    random_walker::infrastructure::flush_logging();
    return exit_code;
}
