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
    class LoggingShutdownGuard final {
    public:
        ~LoggingShutdownGuard() {
            random_walker::infrastructure::shutdown_logging();
        }
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
    }

    int exit_code = 0;
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
            exit_code = -1;
        } else {
            random_walker::application::log_info(
                random_walker::application::log_category::application
                , "QML root object loaded"
            );
            exit_code = app.exec();
            random_walker::application::log_info(
                random_walker::application::log_category::application
                , "Application event loop finished"
            );
        }
    }

    random_walker::application::log_info(
        random_walker::application::log_category::application
        , "Application shutdown"
    );
    return exit_code;
}
