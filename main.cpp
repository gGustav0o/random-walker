#include <memory>
#include <string>

#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "bootstrap/AppContext.hpp"
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

    const bool logging_ready =
        !random_walker::infrastructure::initialize_logging().has_value();
    const LoggingShutdownGuard logging_shutdown_guard;
    if (logging_ready) {
        random_walker::infrastructure::log_info(
            "application"
            , std::string("Logging initialized at ")
                + random_walker::infrastructure::active_log_file_path().string()
        );
        random_walker::infrastructure::log_info(
            "application"
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
            "qrc:/qt/qml/random-walker/view/views/MainView.qml")));
        if (engine.rootObjects().isEmpty()) {
            random_walker::infrastructure::log_error(
                "application"
                , "QML root object creation failed"
            );
            exit_code = -1;
        } else {
            random_walker::infrastructure::log_info(
                "application"
                , "QML root object loaded"
            );
            exit_code = app.exec();
            random_walker::infrastructure::log_info(
                "application"
                , "Application event loop finished"
            );
        }
    }

    random_walker::infrastructure::log_info(
        "application"
        , "Application shutdown"
    );
    return exit_code;
}
