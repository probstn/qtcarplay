#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>

#include <optional>

namespace CarplayProtocol {

constexpr quint32 Sync = 0x55aa55aa;
constexpr quint16 VendorId = 0x1314;
constexpr quint16 ProductId = 0x1521;
constexpr qsizetype HeaderSize = 16;

enum class MessageType : quint32 {
    Open = 0x01,
    Plugged = 0x02,
    Phase = 0x03,
    Unplugged = 0x04,
    Touch = 0x05,
    VideoData = 0x06,
    AudioData = 0x07,
    Command = 0x08,
    LogoType = 0x09,
    BluetoothAddress = 0x0a,
    BluetoothPIN = 0x0c,
    BluetoothDeviceName = 0x0d,
    WifiDeviceName = 0x0e,
    DisconnectPhone = 0x0f,
    BluetoothPairedList = 0x12,
    ManufacturerInfo = 0x14,
    CloseDongle = 0x15,
    MultiTouch = 0x17,
    HiCarLink = 0x18,
    BoxSettings = 0x19,
    MediaData = 0x2a,
    SendFile = 0x99,
    HeartBeat = 0xaa,
    SoftwareVersion = 0xcc,
};

enum class Command : quint32 {
    Invalid = 0,
    StartRecordAudio = 1,
    StopRecordAudio = 2,
    RequestHostUi = 3,
    Siri = 5,
    Mic = 7,
    Frame = 12,
    BoxMic = 15,
    AudioTransferOn = 22,
    AudioTransferOff = 23,
    Wifi24G = 24,
    Wifi5G = 25,
    Left = 100,
    Right = 101,
    SelectDown = 104,
    SelectUp = 105,
    Back = 106,
    Up = 113,
    Down = 114,
    Home = 200,
    Play = 201,
    Pause = 202,
    Next = 204,
    Previous = 205,
    WifiEnable = 1000,
    WifiConnect = 1002,
    WifiPair = 1012,
};

enum class TouchAction : quint32 {
    Down = 14,
    Move = 15,
    Up = 16,
};

enum class HandDriveType : quint32 {
    Left = 0,
    Right = 1,
};

struct DongleConfig {
    int width = 1024;
    int height = 600;
    int fps = 120;
    int dpi = 160;
    int format = 5;
    int iBoxVersion = 2;
    int packetMax = 49152;
    int phoneWorkMode = 2;
    int mediaDelay = 0;
    bool nightMode = false;
    bool audioTransferMode = false;
    bool wifi5G = true;
    bool boxMicrophone = false;
    QString boxName = QStringLiteral("QtCarplay");
    HandDriveType handDrive = HandDriveType::Left;
    std::optional<bool> androidWorkMode;
};

struct Header {
    quint32 length = 0;
    MessageType type = MessageType::Open;
};

struct VideoPacket {
    int width = 0;
    int height = 0;
    quint32 flags = 0;
    quint32 declaredLength = 0;
    QByteArray h264;
};

struct AudioPacket {
    int decodeType = 0;
    float volume = 1.0f;
    int audioType = 0;
    int command = 0;
    float volumeDuration = -1.0f;
    QByteArray pcm;
};

QByteArray makeMessage(MessageType type, const QByteArray &payload = {});
std::optional<Header> parseHeader(const QByteArray &header);

QByteArray makeOpen(const DongleConfig &config);
QByteArray makeBoxSettings(const DongleConfig &config);
QByteArray makeCommand(Command command);
QByteArray makeHeartbeat();
QByteArray makeTouch(double x, double y, TouchAction action);
QByteArray makeMicrophoneAudio(const QByteArray &pcm16le);
QByteArray makeFileNumber(const QString &path, quint32 value);
QByteArray makeFileString(const QString &path, const QString &value);

VideoPacket parseVideoPacket(const QByteArray &payload);
AudioPacket parseAudioPacket(const QByteArray &payload);
int parseCommand(const QByteArray &payload);

QList<QByteArray> makeStartupMessages(const DongleConfig &config);

} // namespace CarplayProtocol

Q_DECLARE_METATYPE(CarplayProtocol::Header)
