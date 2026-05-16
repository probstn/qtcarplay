#include "../src/CarplayController.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QTimer>
#include <QVideoSink>

#include <iostream>

namespace {

QString prefix()
{
    return QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss.zzz"));
}

void logLine(const QString &line)
{
    std::cout << prefix().toStdString() << " " << line.toStdString() << std::endl;
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    const int durationMs = argc > 1 ? std::max(5000, QString::fromLocal8Bit(argv[1]).toInt() * 1000) : 45000;
    const int fps = argc > 2 ? std::clamp(QString::fromLocal8Bit(argv[2]).toInt(), 15, 120) : 60;

    CarplayController controller;
    QVideoSink sink;
    int sinkFrames = 0;

    controller.setVideoSink(&sink);

    QObject::connect(&controller, &CarplayController::statusChanged, [&]() {
        logLine(QString("status: %1").arg(controller.status()));
    });
    QObject::connect(&controller, &CarplayController::dongleReadyChanged, [&]() {
        logLine(QString("ready: %1").arg(controller.dongleReady() ? QStringLiteral("true") : QStringLiteral("false")));
    });
    QObject::connect(&controller, &CarplayController::receivingVideoChanged, [&]() {
        logLine(QString("receivingVideo: %1").arg(controller.receivingVideo() ? QStringLiteral("true") : QStringLiteral("false")));
    });
    QObject::connect(&controller, &CarplayController::videoGeometryChanged, [&]() {
        logLine(QString("geometry: %1x%2").arg(controller.videoWidth()).arg(controller.videoHeight()));
    });
    QObject::connect(&controller, &CarplayController::statsChanged, [&]() {
        if (controller.frameCount() > 0 && controller.frameCount() % 30 == 0) {
            logLine(QString("decoded: controllerFrames=%1 sinkFrames=%2 fps=%3")
                        .arg(controller.frameCount())
                        .arg(sinkFrames)
                        .arg(controller.decodedFps(), 0, 'f', 1));
        }
    });
    QObject::connect(&sink, &QVideoSink::videoFrameChanged, [&]() {
        ++sinkFrames;
        if (sinkFrames <= 3 || sinkFrames % 30 == 0)
            logLine(QString("sink frame #%1 valid=%2").arg(sinkFrames).arg(sink.videoFrame().isValid()));
    });

    QTimer::singleShot(durationMs, [&]() {
        logLine(QString("summary: receiving=%1 controllerFrames=%2 sinkFrames=%3 decodedFps=%4")
                    .arg(controller.receivingVideo() ? QStringLiteral("true") : QStringLiteral("false"))
                    .arg(controller.frameCount())
                    .arg(sinkFrames)
                    .arg(controller.decodedFps(), 0, 'f', 1));
        controller.stop();
        app.quit();
    });

    logLine(QString("starting controller probe for %1 seconds at %2 fps").arg(durationMs / 1000).arg(fps));
    controller.startStream(1024, 600, fps);
    return app.exec();
}
