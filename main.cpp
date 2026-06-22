#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "app/context/AppContext.hpp"

static void setup_high_dpi()
{
#if defined(Q_OS_WIN) && QT_VERSION_CHECK(5, 6, 0) <= QT_VERSION && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
}

int main(int argc, char* argv[])
{
    setup_high_dpi();
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    AppContext context(engine);

    engine.load(QUrl(QStringLiteral(
        "qrc:/qt/qml/random-walker/view/views/MainView.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
