#include "CarplayProtocol.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QtEndian>

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace CarplayProtocol {

namespace {

void appendU32(QByteArray &out, quint32 value)
{
    const quint32 le = qToLittleEndian(value);
    out.append(reinterpret_cast<const char *>(&le), sizeof(le));
}

quint32 readU32(const QByteArray &data, qsizetype offset)
{
    if (offset < 0 || offset + 4 > data.size())
        throw std::out_of_range("readU32 out of range");
    return qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(data.constData() + offset));
}

float readFloat(const QByteArray &data, qsizetype offset)
{
    const quint32 raw = readU32(data, offset);
    float value = 0.0f;
    std::memcpy(&value, &raw, sizeof(value));
    return value;
}

void appendFloat(QByteArray &out, float value)
{
    quint32 raw = 0;
    std::memcpy(&raw, &value, sizeof(raw));
    appendU32(out, raw);
}

QByteArray filePayload(const QString &path, const QByteArray &content)
{
    QByteArray name = path.toLatin1();
    name.append('\0');

    QByteArray payload;
    payload.reserve(8 + name.size() + content.size());
    appendU32(payload, static_cast<quint32>(name.size()));
    payload.append(name);
    appendU32(payload, static_cast<quint32>(content.size()));
    payload.append(content);
    return payload;
}

QByteArray makeNumberContent(quint32 value)
{
    QByteArray content;
    appendU32(content, value);
    return content;
}

} // namespace

QByteArray makeMessage(MessageType type, const QByteArray &payload)
{
    QByteArray message;
    message.reserve(HeaderSize + payload.size());
    appendU32(message, Sync);
    appendU32(message, static_cast<quint32>(payload.size()));
    appendU32(message, static_cast<quint32>(type));
    appendU32(message, ~static_cast<quint32>(type));
    message.append(payload);
    return message;
}

std::optional<Header> parseHeader(const QByteArray &header)
{
    if (header.size() != HeaderSize)
        return std::nullopt;

    const quint32 sync = readU32(header, 0);
    const quint32 length = readU32(header, 4);
    const quint32 type = readU32(header, 8);
    const quint32 invertedType = readU32(header, 12);

    if (sync != Sync || invertedType != ~type)
        return std::nullopt;

    return Header{length, static_cast<MessageType>(type)};
}

QByteArray makeOpen(const DongleConfig &config)
{
    QByteArray payload;
    payload.reserve(7 * 4);
    appendU32(payload, static_cast<quint32>(config.width));
    appendU32(payload, static_cast<quint32>(config.height));
    appendU32(payload, static_cast<quint32>(config.fps));
    appendU32(payload, static_cast<quint32>(config.format));
    appendU32(payload, static_cast<quint32>(config.packetMax));
    appendU32(payload, static_cast<quint32>(config.iBoxVersion));
    appendU32(payload, static_cast<quint32>(config.phoneWorkMode));
    return makeMessage(MessageType::Open, payload);
}

QByteArray makeBoxSettings(const DongleConfig &config)
{
    QJsonObject settings;
    settings.insert(QStringLiteral("mediaDelay"), config.mediaDelay);
    settings.insert(QStringLiteral("syncTime"), QDateTime::currentMSecsSinceEpoch());
    settings.insert(QStringLiteral("androidAutoSizeW"), config.width);
    settings.insert(QStringLiteral("androidAutoSizeH"), config.height);
    return makeMessage(MessageType::BoxSettings, QJsonDocument(settings).toJson(QJsonDocument::Compact));
}

QByteArray makeCommand(Command command)
{
    QByteArray payload;
    appendU32(payload, static_cast<quint32>(command));
    return makeMessage(MessageType::Command, payload);
}

QByteArray makeHeartbeat()
{
    return makeMessage(MessageType::HeartBeat);
}

QByteArray makeTouch(double x, double y, TouchAction action)
{
    const auto clampedX = static_cast<quint32>(std::clamp(x, 0.0, 1.0) * 10000.0);
    const auto clampedY = static_cast<quint32>(std::clamp(y, 0.0, 1.0) * 10000.0);

    QByteArray payload;
    payload.reserve(16);
    appendU32(payload, static_cast<quint32>(action));
    appendU32(payload, clampedX);
    appendU32(payload, clampedY);
    appendU32(payload, 0);
    return makeMessage(MessageType::Touch, payload);
}

QByteArray makeMicrophoneAudio(const QByteArray &pcm16le)
{
    QByteArray payload;
    payload.reserve(12 + pcm16le.size());
    appendU32(payload, 5);
    appendFloat(payload, 0.0f);
    appendU32(payload, 3);
    payload.append(pcm16le);
    return makeMessage(MessageType::AudioData, payload);
}

QByteArray makeFileNumber(const QString &path, quint32 value)
{
    return makeMessage(MessageType::SendFile, filePayload(path, makeNumberContent(value)));
}

QByteArray makeFileString(const QString &path, const QString &value)
{
    return makeMessage(MessageType::SendFile, filePayload(path, value.left(16).toLatin1()));
}

VideoPacket parseVideoPacket(const QByteArray &payload)
{
    if (payload.size() < 20)
        throw std::runtime_error("Video packet too short");

    VideoPacket packet;
    packet.width = static_cast<int>(readU32(payload, 0));
    packet.height = static_cast<int>(readU32(payload, 4));
    packet.flags = readU32(payload, 8);
    packet.declaredLength = readU32(payload, 12);

    // Use all bytes after the dongle video header. Some firmwares report a
    // declared length that does not include all NAL data in this USB message.
    packet.h264 = payload.mid(20);
    return packet;
}

AudioPacket parseAudioPacket(const QByteArray &payload)
{
    if (payload.size() < 12)
        throw std::runtime_error("Audio packet too short");

    AudioPacket packet;
    packet.decodeType = static_cast<int>(readU32(payload, 0));
    packet.volume = readFloat(payload, 4);
    packet.audioType = static_cast<int>(readU32(payload, 8));

    const qsizetype amount = payload.size() - 12;
    if (amount == 1) {
        packet.command = static_cast<qint8>(payload.at(12));
    } else if (amount == 4) {
        packet.volumeDuration = readFloat(payload, 12);
    } else if (amount > 0) {
        packet.pcm = payload.mid(12);
    }

    return packet;
}

PluggedPacket parsePluggedPacket(const QByteArray &payload)
{
    if (payload.size() < 4)
        throw std::runtime_error("Plugged packet too short");

    PluggedPacket packet;
    packet.phoneType = static_cast<int>(readU32(payload, 0));
    if (payload.size() >= 8)
        packet.wifi = static_cast<int>(readU32(payload, 4));
    return packet;
}

int parseCommand(const QByteArray &payload)
{
    if (payload.size() < 4)
        return 0;
    return static_cast<int>(readU32(payload, 0));
}

QList<QByteArray> makeStartupMessages(const DongleConfig &config)
{
    QList<QByteArray> messages;
    messages.reserve(12);
    messages.append(makeFileNumber(QStringLiteral("/tmp/screen_dpi"), static_cast<quint32>(config.dpi)));
    messages.append(makeOpen(config));
    messages.append(makeFileNumber(QStringLiteral("/tmp/night_mode"), config.nightMode ? 1 : 0));
    messages.append(makeFileNumber(QStringLiteral("/tmp/hand_drive_mode"), static_cast<quint32>(config.handDrive)));
    messages.append(makeFileNumber(QStringLiteral("/tmp/charge_mode"), 1));
    messages.append(makeFileString(QStringLiteral("/etc/box_name"), config.boxName));
    messages.append(makeBoxSettings(config));
    messages.append(makeCommand(Command::WifiEnable));
    messages.append(makeCommand(config.wifi5G ? Command::Wifi5G : Command::Wifi24G));
    messages.append(makeCommand(config.boxMicrophone ? Command::BoxMic : Command::Mic));
    messages.append(makeCommand(config.audioTransferMode ? Command::AudioTransferOn : Command::AudioTransferOff));
    if (config.androidWorkMode.has_value())
        messages.append(makeFileNumber(QStringLiteral("/etc/android_work_mode"), config.androidWorkMode.value() ? 1 : 0));
    return messages;
}

} // namespace CarplayProtocol
