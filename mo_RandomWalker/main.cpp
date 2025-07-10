#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "app/context/AppContext.hpp"
#include "app/qml/qml_registration.hpp"

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

    register_qml_types();

    AppContext ctx;
    ctx.initialize(&engine);

    ctx.get_engine()->load(QUrl(QStringLiteral("qrc:/qt/qml/mo_randomwalker/view/views/MainView.qml")));
    if (ctx.get_engine()->rootObjects().isEmpty())
        return -1;

    return app.exec();
}
