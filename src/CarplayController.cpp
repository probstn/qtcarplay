#include "CarplayController.h"

#include "H264Decoder.h"
#include "PcmRingBuffer.h"
#include "UsbDongleTransport.h"

#include <QAudioDevice>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfoList>
#include <QMediaDevices>
#include <QMetaObject>
#include <QTimer>
#include <QVideoFrame>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iterator>

namespace {

constexpr int WatchdogIntervalMs = 5000;
constexpr int NoVideoRestartMs = 45000;
constexpr int StalledFrameRestartTicks = 4;

QString phoneTypeName(int phoneType)
{
    switch (phoneType) {
    case 1:
        return QStringLiteral("Android Mirror");
    case 3:
        return QStringLiteral("CarPlay");
    case 4:
        return QStringLiteral("iPhone Mirror");
    case 5:
        return QStringLiteral("Android Auto");
    case 6:
        return QStringLiteral("HiCar");
    default:
        return QString("phone type %1").arg(phoneType);
    }
}

QAudioFormat audioFormatForDecodeType(int decodeType)
{
    QAudioFormat format;
    format.setSampleFormat(QAudioFormat::Int16);

    switch (decodeType) {
    case 1:
    case 2:
        format.setSampleRate(44100);
        format.setChannelCount(2);
        break;
    case 3:
        format.setSampleRate(8000);
        format.setChannelCount(1);
        break;
    case 4:
        format.setSampleRate(48000);
        format.setChannelCount(2);
        break;
    case 5:
        format.setSampleRate(16000);
        format.setChannelCount(1);
        break;
    case 6:
        format.setSampleRate(24000);
        format.setChannelCount(1);
        break;
    case 7:
        format.setSampleRate(16000);
        format.setChannelCount(2);
        break;
    default:
        format.setSampleRate(44100);
        format.setChannelCount(2);
        break;
    }

    return format;
}

QAudioDevice selectedAudioOutput()
{
    const QList<QAudioDevice> outputs = QMediaDevices::audioOutputs();
    const QString requested = qEnvironmentVariable("QTCARPLAY_AUDIO_OUTPUT_ID");

    if (!requested.isEmpty()) {
        for (const QAudioDevice &output : outputs) {
            if (QString::fromUtf8(output.id()) == requested || output.description().contains(requested, Qt::CaseInsensitive))
                return output;
        }

        qWarning().noquote() << "Requested audio output not found:" << requested;
        for (const QAudioDevice &output : outputs) {
            qWarning().noquote() << "Available audio output:"
                                 << output.description()
                                 << "id=" << QString::fromUtf8(output.id());
        }
    }

    return QMediaDevices::defaultAudioOutput();
}

} // namespace

CarplayController::CarplayController(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<QVideoFrame>();
    qRegisterMetaType<CarplayProtocol::Header>();

    m_frameRequestTimer = new QTimer(this);
    m_frameRequestTimer->setInterval(5000);
    connect(m_frameRequestTimer, &QTimer::timeout, this, [this]() {
        if (m_transport && streaming())
            m_transport->sendMessage(CarplayProtocol::makeCommand(CarplayProtocol::Command::Frame));
    });

    m_watchdogTimer = new QTimer(this);
    m_watchdogTimer->setInterval(WatchdogIntervalMs);
    connect(m_watchdogTimer, &QTimer::timeout, this, &CarplayController::watchdogTick);
}

CarplayController::~CarplayController()
{
    stop();
}

QString CarplayController::status() const
{
    std::scoped_lock lock(m_stateMutex);
    return m_status;
}

bool CarplayController::dongleReady() const
{
    std::scoped_lock lock(m_stateMutex);
    return m_dongleReady;
}

bool CarplayController::streaming() const
{
    std::scoped_lock lock(m_stateMutex);
    return m_streaming;
}

bool CarplayController::receivingVideo() const
{
    std::scoped_lock lock(m_stateMutex);
    return m_receivingVideo;
}

int CarplayController::frameCount() const
{
    std::scoped_lock lock(m_stateMutex);
    return m_frameCount;
}

double CarplayController::decodedFps() const
{
    std::scoped_lock lock(m_stateMutex);
    return m_decodedFps;
}

int CarplayController::videoWidth() const
{
    std::scoped_lock lock(m_stateMutex);
    return m_videoWidth;
}

int CarplayController::videoHeight() const
{
    std::scoped_lock lock(m_stateMutex);
    return m_videoHeight;
}

bool CarplayController::audioActive() const
{
    std::scoped_lock lock(m_stateMutex);
    return m_audioActive;
}

int CarplayController::audioPacketCount() const
{
    std::scoped_lock lock(m_stateMutex);
    return m_audioPacketCount;
}

QVideoSink *CarplayController::videoSink() const
{
    return m_videoSink;
}

void CarplayController::setVideoSink(QVideoSink *sink)
{
    if (m_videoSink == sink)
        return;
    m_videoSink = sink;
    emit videoSinkChanged();
}

void CarplayController::verifyHardware()
{
    setStatus(QStringLiteral("USB verification happens on Start Live with the new transport"));
}

void CarplayController::startStream(int width, int height, int fps)
{
    stop();
    resetStats();

    CarplayProtocol::DongleConfig config;
    config.width = std::max(320, width);
    config.height = std::max(240, height);
    config.fps = std::clamp(fps, 15, 120);
    config.mediaDelay = 0;
    config.packetMax = 49152;
    config.boxName = QStringLiteral("QtCarplay");
    config.audioTransferMode = false;
    m_lastConfig = config;
    m_streamTimer.restart();
    m_lastWatchdogFrameCount = 0;
    m_stalledWatchdogTicks = 0;
    m_restartPending = false;

    startDecoder(config.fps);

    m_transport = new UsbDongleTransport(this);
    connect(m_transport, &UsbDongleTransport::statusChanged, this, &CarplayController::setStatus);
    connect(m_transport, &UsbDongleTransport::dongleReadyChanged, this, &CarplayController::setDongleReady);
    connect(m_transport, &UsbDongleTransport::messageReceived, this, &CarplayController::handleTransportMessage);
    connect(m_transport, &UsbDongleTransport::transportFailed, this, [this](const QString &reason) {
        setStatus(QStringLiteral("USB transport failed: ") + reason);
        setStreaming(false);
        setDongleReady(false);
        stopDecoder();
    });
    connect(m_transport, &QThread::finished, this, [this, transport = m_transport]() {
        if (m_transport == transport)
            m_transport = nullptr;
    });
    connect(m_transport, &QThread::finished, m_transport, &QObject::deleteLater);

    setStreaming(true);
    setStatus(QStringLiteral("Starting CarPlay transport"));
    startWatchdog();
    m_transport->startTransport(config);
}

void CarplayController::stop()
{
    m_captureRunning = false;
    if (m_captureThread.joinable())
        m_captureThread.join();

    if (m_transport) {
        UsbDongleTransport *transport = m_transport;
        m_transport = nullptr;
        transport->stopTransport();
    }

    stopDecoder();
    stopAudio();
    stopFrameRequests();
    stopWatchdog();
    m_restartPending = false;
    setStreaming(false);
    setReceivingVideo(false);
}

void CarplayController::playCapture(const QString &captureDirectory, int fps)
{
    stop();
    resetStats();
    startDecoder(fps);
    setStreaming(true);
    setStatus(QStringLiteral("Playing saved H264 capture"));

    m_captureRunning = true;
    m_captureThread = std::thread(&CarplayController::captureLoop, this, captureDirectory, std::clamp(fps, 1, 120));
}

void CarplayController::touchDown(double x, double y)
{
    sendTouch(x, y, CarplayProtocol::TouchAction::Down);
}

void CarplayController::touchMove(double x, double y)
{
    sendTouch(x, y, CarplayProtocol::TouchAction::Move);
}

void CarplayController::touchUp(double x, double y)
{
    sendTouch(x, y, CarplayProtocol::TouchAction::Up);
}

void CarplayController::setStatus(const QString &status)
{
    {
        std::scoped_lock lock(m_stateMutex);
        if (m_status == status)
            return;
        m_status = status;
    }
    qInfo().noquote() << "CarPlay status:" << status;
    emit statusChanged();
}

void CarplayController::setDongleReady(bool ready)
{
    {
        std::scoped_lock lock(m_stateMutex);
        if (m_dongleReady == ready)
            return;
        m_dongleReady = ready;
    }
    emit dongleReadyChanged();
}

void CarplayController::setStreaming(bool streaming)
{
    {
        std::scoped_lock lock(m_stateMutex);
        if (m_streaming == streaming)
            return;
        m_streaming = streaming;
    }
    emit streamingChanged();
}

void CarplayController::setReceivingVideo(bool receiving)
{
    {
        std::scoped_lock lock(m_stateMutex);
        if (m_receivingVideo == receiving)
            return;
        m_receivingVideo = receiving;
    }
    emit receivingVideoChanged();
}

void CarplayController::setVideoGeometry(int width, int height)
{
    {
        std::scoped_lock lock(m_stateMutex);
        if (m_videoWidth == width && m_videoHeight == height)
            return;
        m_videoWidth = width;
        m_videoHeight = height;
    }
    emit videoGeometryChanged();
}

void CarplayController::setAudioActive(bool active)
{
    {
        std::scoped_lock lock(m_stateMutex);
        if (m_audioActive == active)
            return;
        m_audioActive = active;
    }
    emit audioActiveChanged();
}

void CarplayController::resetStats()
{
    {
        std::scoped_lock lock(m_stateMutex);
        m_frameCount = 0;
        m_decodedFps = 0.0;
        m_videoWidth = 0;
        m_videoHeight = 0;
        m_audioPacketCount = 0;
    }
    emit statsChanged();
    emit videoGeometryChanged();
    emit audioStatsChanged();
}

void CarplayController::startDecoder(int fps)
{
    m_decoderRunning = true;
    m_decoderThread = std::thread(&CarplayController::decoderLoop, this, std::clamp(fps, 1, 120));
}

void CarplayController::stopDecoder()
{
    m_decoderRunning = false;
    m_queueCondition.notify_all();
    if (m_decoderThread.joinable())
        m_decoderThread.join();

    {
        std::scoped_lock lock(m_queueMutex);
        m_decodeQueue.clear();
    }
    {
        std::scoped_lock lock(m_presentMutex);
        m_pendingFrame = {};
        m_presentationScheduled = false;
    }
}

void CarplayController::enqueueEncodedFrame(std::vector<uint8_t> bytes, int width, int height)
{
    setReceivingVideo(true);
    setVideoGeometry(width, height);

    {
        std::scoped_lock lock(m_queueMutex);
        m_decodeQueue.push_back({std::move(bytes), width, height});

        // Never drop ordinary compressed packets: H264 delta frames depend on
        // prior packets. If the decoder falls catastrophically behind, reset at
        // the next app-level start rather than corrupting the prediction chain.
        if (m_decodeQueue.size() > 240)
            m_decodeQueue.pop_front();
    }
    m_queueCondition.notify_one();
}

void CarplayController::decoderLoop(int fps)
{
    H264Decoder decoder(fps);
    QElapsedTimer timer;
    timer.start();
    int decodedSinceUpdate = 0;
    qint64 lastUpdateMs = timer.elapsed();

    while (m_decoderRunning) {
        EncodedFrame encoded;
        {
            std::unique_lock lock(m_queueMutex);
            m_queueCondition.wait(lock, [this]() {
                return !m_decoderRunning || !m_decodeQueue.empty();
            });
            if (!m_decoderRunning)
                break;
            encoded = std::move(m_decodeQueue.front());
            m_decodeQueue.pop_front();
        }

        try {
            const auto frames = decoder.decode(encoded.bytes.data(), static_cast<qsizetype>(encoded.bytes.size()));
            for (const QVideoFrame &frame : frames) {
                scheduleFramePresentation(frame);
                ++decodedSinceUpdate;

                const qint64 nowMs = timer.elapsed();
                if (nowMs - lastUpdateMs >= 500) {
                    const double fpsValue = decodedSinceUpdate * 1000.0 / (nowMs - lastUpdateMs);
                    decodedSinceUpdate = 0;
                    lastUpdateMs = nowMs;
                    QMetaObject::invokeMethod(this, [this, fpsValue]() {
                        {
                            std::scoped_lock lock(m_stateMutex);
                            m_decodedFps = fpsValue;
                        }
                        emit statsChanged();
                    }, Qt::QueuedConnection);
                }
            }
        } catch (const std::exception &error) {
            QMetaObject::invokeMethod(this, [this, message = QString("Decode warning: %1").arg(error.what())]() {
                setStatus(message);
            }, Qt::QueuedConnection);
        }
    }
}

void CarplayController::scheduleFramePresentation(const QVideoFrame &frame)
{
    bool shouldSchedule = false;
    {
        std::scoped_lock lock(m_presentMutex);
        m_pendingFrame = frame;
        shouldSchedule = !m_presentationScheduled;
        m_presentationScheduled = true;
    }

    if (!shouldSchedule)
        return;

    QMetaObject::invokeMethod(this, [this]() {
        QVideoFrame frame;
        {
            std::scoped_lock lock(m_presentMutex);
            frame = m_pendingFrame;
            m_pendingFrame = {};
            m_presentationScheduled = false;
        }

        if (frame.isValid() && m_videoSink) {
            m_videoSink->setVideoFrame(frame);
            {
                std::scoped_lock lock(m_stateMutex);
                ++m_frameCount;
            }
            emit statsChanged();
        }
    }, Qt::QueuedConnection);
}

void CarplayController::presentFrame(const QVideoFrame &frame)
{
    scheduleFramePresentation(frame);
}

void CarplayController::captureLoop(QString captureDirectory, int fps)
{
    QDir dir(captureDirectory);
    const QFileInfoList entries = dir.entryInfoList({"*.h264"}, QDir::Files, QDir::Name);
    if (entries.isEmpty()) {
        QMetaObject::invokeMethod(this, [this, captureDirectory]() {
            setStatus(QString("No .h264 capture frames found in %1").arg(captureDirectory));
        }, Qt::QueuedConnection);
        return;
    }

    const auto frameDuration = std::chrono::milliseconds(1000 / std::max(1, fps));
    qsizetype index = 0;
    while (m_captureRunning) {
        const QFileInfo &entry = entries.at(index % entries.size());
        std::ifstream file(entry.absoluteFilePath().toStdString(), std::ios::binary);
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (!bytes.empty()) {
            QMetaObject::invokeMethod(this, [this, bytes = std::move(bytes)]() mutable {
                enqueueEncodedFrame(std::move(bytes), 1920, 1080);
            }, Qt::QueuedConnection);
        }

        ++index;
        std::this_thread::sleep_for(frameDuration);
    }
}

void CarplayController::handleTransportMessage(CarplayProtocol::Header header, const QByteArray &payload)
{
    try {
        switch (header.type) {
        case CarplayProtocol::MessageType::Plugged:
            handlePluggedPacket(CarplayProtocol::parsePluggedPacket(payload));
            break;
        case CarplayProtocol::MessageType::Unplugged:
            stopFrameRequests();
            setReceivingVideo(false);
            setStatus(QStringLiteral("Phone disconnected"));
            break;
        case CarplayProtocol::MessageType::VideoData:
            handleVideoPacket(CarplayProtocol::parseVideoPacket(payload));
            break;
        case CarplayProtocol::MessageType::AudioData:
            handleAudioPacket(CarplayProtocol::parseAudioPacket(payload));
            break;
        case CarplayProtocol::MessageType::Command:
            handleDongleCommand(CarplayProtocol::parseCommand(payload));
            break;
        default:
            break;
        }
    } catch (const std::exception &error) {
        setStatus(QString("Protocol warning: %1").arg(error.what()));
    }
}

void CarplayController::handlePluggedPacket(const CarplayProtocol::PluggedPacket &packet)
{
    const QString phoneName = phoneTypeName(packet.phoneType);
    if (packet.wifi.has_value()) {
        setStatus(QString("%1 connected, wifi %2")
                      .arg(phoneName)
                      .arg(packet.wifi.value() ? QStringLiteral("available") : QStringLiteral("unavailable")));
    } else {
        setStatus(QString("%1 connected").arg(phoneName));
    }

    startFrameRequests();
}

void CarplayController::handleVideoPacket(const CarplayProtocol::VideoPacket &packet)
{
    startFrameRequests();
    std::vector<uint8_t> bytes(static_cast<size_t>(packet.h264.size()));
    std::copy(packet.h264.begin(), packet.h264.end(), bytes.begin());
    enqueueEncodedFrame(std::move(bytes), packet.width, packet.height);
}

void CarplayController::handleAudioPacket(const CarplayProtocol::AudioPacket &packet)
{
    if (packet.command != 0) {
        switch (packet.command) {
        case 4:
        case 8:
            startMicrophone();
            break;
        case 5:
        case 9:
            stopMicrophone();
            break;
        default:
            break;
        }
        return;
    }

    if (!packet.pcm.isEmpty())
        writeAudio(packet.decodeType, packet.audioType, packet.volume, packet.pcm);
}

void CarplayController::handleDongleCommand(int command)
{
    switch (static_cast<CarplayProtocol::Command>(command)) {
    case CarplayProtocol::Command::StartRecordAudio:
        startMicrophone();
        break;
    case CarplayProtocol::Command::StopRecordAudio:
        stopMicrophone();
        break;
    default:
        break;
    }
}

void CarplayController::startFrameRequests()
{
    if (m_frameRequestTimer && m_frameRequestTimer->isActive())
        return;

    if (m_transport && streaming())
        m_transport->sendMessage(CarplayProtocol::makeCommand(CarplayProtocol::Command::Frame));
    if (m_frameRequestTimer)
        m_frameRequestTimer->start();
}

void CarplayController::startWatchdog()
{
    if (m_watchdogTimer && !m_watchdogTimer->isActive())
        m_watchdogTimer->start();
}

void CarplayController::stopWatchdog()
{
    if (m_watchdogTimer)
        m_watchdogTimer->stop();
}

void CarplayController::watchdogTick()
{
    if (!streaming() || m_restartPending)
        return;

    const int frames = frameCount();
    const bool hasVideo = receivingVideo();

    if (!hasVideo && m_streamTimer.isValid() && m_streamTimer.elapsed() > NoVideoRestartMs) {
        restartStream(QStringLiteral("No video after connection timeout"));
        return;
    }

    if (!hasVideo) {
        m_stalledWatchdogTicks = 0;
        return;
    }

    if (frames > m_lastWatchdogFrameCount) {
        m_lastWatchdogFrameCount = frames;
        m_stalledWatchdogTicks = 0;
        return;
    }

    if (frames == 0)
        return;

    if (++m_stalledWatchdogTicks >= StalledFrameRestartTicks)
        restartStream(QStringLiteral("Video stalled; reconnecting"));
}

void CarplayController::restartStream(const QString &reason)
{
    if (m_restartPending)
        return;

    m_restartPending = true;
    const CarplayProtocol::DongleConfig config = m_lastConfig;
    setStatus(reason);

    QTimer::singleShot(0, this, [this, config]() {
        stop();
        resetStats();
        m_lastConfig = config;
        m_streamTimer.restart();
        m_lastWatchdogFrameCount = 0;
        m_stalledWatchdogTicks = 0;
        m_restartPending = false;

        startDecoder(config.fps);

        m_transport = new UsbDongleTransport(this);
        connect(m_transport, &UsbDongleTransport::statusChanged, this, &CarplayController::setStatus);
        connect(m_transport, &UsbDongleTransport::dongleReadyChanged, this, &CarplayController::setDongleReady);
        connect(m_transport, &UsbDongleTransport::messageReceived, this, &CarplayController::handleTransportMessage);
        connect(m_transport, &UsbDongleTransport::transportFailed, this, [this](const QString &failureReason) {
            setStatus(QStringLiteral("USB transport failed: ") + failureReason);
            setStreaming(false);
            setDongleReady(false);
            stopDecoder();
        });
        connect(m_transport, &QThread::finished, this, [this, transport = m_transport]() {
            if (m_transport == transport)
                m_transport = nullptr;
        });
        connect(m_transport, &QThread::finished, m_transport, &QObject::deleteLater);

        setStreaming(true);
        setStatus(QStringLiteral("Restarting CarPlay transport"));
        startWatchdog();
        m_transport->startTransport(config);
    });
}

void CarplayController::stopFrameRequests()
{
    if (m_frameRequestTimer)
        m_frameRequestTimer->stop();
}

void CarplayController::writeAudio(int decodeType, int audioType, float volume, const QByteArray &pcm)
{
    {
        std::scoped_lock lock(m_stateMutex);
        ++m_audioPacketCount;
    }
    emit audioStatsChanged();

    const QString key = QString("%1:%2").arg(decodeType).arg(audioType);
    auto it = m_audioStreams.find(key);
    if (it == m_audioStreams.end()) {
        const QAudioFormat format = audioFormatForDecodeType(decodeType);
        const QAudioDevice output = selectedAudioOutput();
        if (output.isNull()) {
            setStatus(QStringLiteral("No audio output device"));
            return;
        }
        qInfo().noquote() << "CarPlay audio output:"
                          << output.description()
                          << "id=" << QString::fromUtf8(output.id())
                          << "decodeType=" << decodeType
                          << "audioType=" << audioType
                          << "format=" << QString("%1 Hz/%2 ch/%3 bytes")
                                           .arg(format.sampleRate())
                                           .arg(format.channelCount())
                                           .arg(format.bytesPerFrame());

        AudioStream stream;
        const int bytesPerSecond = format.bytesPerFrame() * format.sampleRate();
        stream.buffer = new PcmRingBuffer(bytesPerSecond / 5, this);
        stream.sink = new QAudioSink(output, format, this);
        connect(stream.sink, &QAudioSink::stateChanged, this, [sink = stream.sink](QAudio::State state) {
            qInfo() << "CarPlay audio sink state:" << state << "error:" << sink->error();
        });
        stream.sink->setBufferSize(bytesPerSecond / 12);
        stream.sink->start(stream.buffer);

        it = m_audioStreams.insert(key, stream);
        setAudioActive(true);
        setStatus(QString("Audio PCM %1 Hz/%2 ch")
                      .arg(format.sampleRate())
                      .arg(format.channelCount()));
    }

    AudioStream &stream = it.value();
    if (!stream.sink || !stream.buffer)
        return;

    const float playbackVolume = volume > 0.001f ? volume : 1.0f;
    stream.sink->setVolume(std::clamp(playbackVolume, 0.0f, 1.0f));
    stream.buffer->push(pcm);
}

void CarplayController::stopAudio()
{
    stopMicrophone();

    for (AudioStream &stream : m_audioStreams) {
        if (stream.sink) {
            stream.sink->stop();
            stream.sink->deleteLater();
        }
        if (stream.buffer)
            stream.buffer->deleteLater();
    }
    m_audioStreams.clear();
    setAudioActive(false);
}

void CarplayController::startMicrophone()
{
    if (m_audioSource)
        return;

    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    const QAudioDevice input = QMediaDevices::defaultAudioInput();
    if (input.isNull()) {
        setStatus(QStringLiteral("No microphone input device"));
        return;
    }

    m_audioSource = new QAudioSource(input, format, this);
    m_audioSource->setBufferSize(format.bytesPerFrame() * format.sampleRate() / 50);
    m_audioInputDevice = m_audioSource->start();
    if (!m_audioInputDevice) {
        m_audioSource->deleteLater();
        m_audioSource = nullptr;
        setStatus(QStringLiteral("Microphone start failed"));
        return;
    }

    connect(m_audioInputDevice, &QIODevice::readyRead, this, [this]() {
        if (!m_audioInputDevice)
            return;
        const QByteArray pcm = m_audioInputDevice->readAll();
        if (!pcm.isEmpty())
            sendMicrophoneData(pcm);
    });

    setStatus(QStringLiteral("Microphone streaming"));
}

void CarplayController::stopMicrophone()
{
    if (!m_audioSource)
        return;

    m_audioSource->stop();
    m_audioSource->deleteLater();
    m_audioSource = nullptr;
    m_audioInputDevice = nullptr;
}

void CarplayController::sendMicrophoneData(const QByteArray &pcm)
{
    if (!m_transport || !streaming())
        return;
    m_transport->sendMessage(CarplayProtocol::makeMicrophoneAudio(pcm));
}

void CarplayController::sendTouch(double x, double y, CarplayProtocol::TouchAction action)
{
    if (!m_transport || !streaming())
        return;
    m_transport->sendMessage(CarplayProtocol::makeTouch(x, y, action));
}
