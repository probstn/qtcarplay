#include "../src/CarplayProtocol.h"
#include "../src/UsbDongleTransport.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QTimer>

#include <iostream>

namespace {

QString messageName(CarplayProtocol::MessageType type)
{
    switch (type) {
    case CarplayProtocol::MessageType::Open:
        return QStringLiteral("Open");
    case CarplayProtocol::MessageType::Plugged:
        return QStringLiteral("Plugged");
    case CarplayProtocol::MessageType::Phase:
        return QStringLiteral("Phase");
    case CarplayProtocol::MessageType::Unplugged:
        return QStringLiteral("Unplugged");
    case CarplayProtocol::MessageType::VideoData:
        return QStringLiteral("VideoData");
    case CarplayProtocol::MessageType::AudioData:
        return QStringLiteral("AudioData");
    case CarplayProtocol::MessageType::Command:
        return QStringLiteral("Command");
    case CarplayProtocol::MessageType::BluetoothAddress:
        return QStringLiteral("BluetoothAddress");
    case CarplayProtocol::MessageType::BluetoothPIN:
        return QStringLiteral("BluetoothPIN");
    case CarplayProtocol::MessageType::BluetoothDeviceName:
        return QStringLiteral("BluetoothDeviceName");
    case CarplayProtocol::MessageType::WifiDeviceName:
        return QStringLiteral("WifiDeviceName");
    case CarplayProtocol::MessageType::BluetoothPairedList:
        return QStringLiteral("BluetoothPairedList");
    case CarplayProtocol::MessageType::BoxSettings:
        return QStringLiteral("BoxSettings");
    case CarplayProtocol::MessageType::MediaData:
        return QStringLiteral("MediaData");
    case CarplayProtocol::MessageType::HeartBeat:
        return QStringLiteral("HeartBeat");
    case CarplayProtocol::MessageType::SoftwareVersion:
        return QStringLiteral("SoftwareVersion");
    default:
        return QString("0x%1").arg(static_cast<quint32>(type), 2, 16, QLatin1Char('0'));
    }
}

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

    CarplayProtocol::DongleConfig config;
    config.width = 1024;
    config.height = 600;
    config.fps = 60;
    config.packetMax = 49152;
    config.mediaDelay = 300;
    config.boxName = QStringLiteral("QtCarplay");

    UsbDongleTransport transport;
    int messages = 0;
    int videos = 0;
    int audios = 0;
    int plugged = 0;

    QObject::connect(&transport, &UsbDongleTransport::statusChanged, [](const QString &status) {
        logLine(QString("status: %1").arg(status));
    });
    QObject::connect(&transport, &UsbDongleTransport::dongleReadyChanged, [](bool ready) {
        logLine(QString("ready: %1").arg(ready ? QStringLiteral("true") : QStringLiteral("false")));
    });
    QObject::connect(&transport, &UsbDongleTransport::transportFailed, [&](const QString &reason) {
        logLine(QString("failed: %1").arg(reason));
        QTimer::singleShot(0, &app, &QCoreApplication::quit);
    });
    QObject::connect(&transport, &UsbDongleTransport::messageReceived, [&](CarplayProtocol::Header header, const QByteArray &payload) {
        ++messages;
        if (header.type == CarplayProtocol::MessageType::VideoData)
            ++videos;
        else if (header.type == CarplayProtocol::MessageType::AudioData)
            ++audios;
        else if (header.type == CarplayProtocol::MessageType::Plugged)
            ++plugged;

        QString detail;
        try {
            if (header.type == CarplayProtocol::MessageType::Plugged) {
                const auto packet = CarplayProtocol::parsePluggedPacket(payload);
                detail = QString(" phoneType=%1").arg(packet.phoneType);
                if (packet.wifi.has_value())
                    detail += QString(" wifi=%1").arg(packet.wifi.value());
            } else if (header.type == CarplayProtocol::MessageType::VideoData) {
                const auto packet = CarplayProtocol::parseVideoPacket(payload);
                detail = QString(" %1x%2 h264=%3 flags=0x%4 declared=%5")
                             .arg(packet.width)
                             .arg(packet.height)
                             .arg(packet.h264.size())
                             .arg(packet.flags, 0, 16)
                             .arg(packet.declaredLength);
            } else if (header.type == CarplayProtocol::MessageType::Command) {
                detail = QString(" value=%1").arg(CarplayProtocol::parseCommand(payload));
            }
        } catch (const std::exception &error) {
            detail = QString(" parse-warning=%1").arg(error.what());
        }

        logLine(QString("message #%1 type=%2 length=%3%4")
                    .arg(messages)
                    .arg(messageName(header.type))
                    .arg(payload.size())
                    .arg(detail));
    });

    QTimer::singleShot(durationMs, [&]() {
        logLine(QString("summary: messages=%1 plugged=%2 video=%3 audio=%4")
                    .arg(messages)
                    .arg(plugged)
                    .arg(videos)
                    .arg(audios));
        transport.stopTransport();
        app.quit();
    });

    logLine(QString("starting session probe for %1 seconds").arg(durationMs / 1000));
    transport.startTransport(config);
    return app.exec();
}
