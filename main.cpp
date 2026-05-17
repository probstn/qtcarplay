#include <QGuiApplication>
#include <QCursor>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "src/CarplayController.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_CompressHighFrequencyEvents, false);
    QCoreApplication::setAttribute(Qt::AA_CompressTabletEvents, false);

    qputenv("QSG_RENDER_LOOP", "threaded");
    if (!qEnvironmentVariableIsSet("QTCARPLAY_HW_DECODE"))
        qputenv("QTCARPLAY_HW_DECODE", "0");

    QGuiApplication app(argc, argv);
    QGuiApplication::setOverrideCursor(Qt::BlankCursor);

    qmlRegisterType<CarplayController>("QtCarplay", 1, 0, "CarplayController");

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("QtCarplay", "Main");

    return QCoreApplication::exec();
}
