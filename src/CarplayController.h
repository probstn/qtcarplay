#pragma once

#include "CarplayProtocol.h"

#include <QAudioFormat>
#include <QAudioSink>
#include <QAudioSource>
#include <QHash>
#include <QIODevice>
#include <QObject>
#include <QPointer>
#include <QQueue>
#include <QVideoFrame>
#include <QVideoSink>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

class PcmRingBuffer;
class UsbDongleTransport;

class CarplayController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool dongleReady READ dongleReady NOTIFY dongleReadyChanged)
    Q_PROPERTY(bool streaming READ streaming NOTIFY streamingChanged)
    Q_PROPERTY(bool receivingVideo READ receivingVideo NOTIFY receivingVideoChanged)
    Q_PROPERTY(int frameCount READ frameCount NOTIFY statsChanged)
    Q_PROPERTY(double decodedFps READ decodedFps NOTIFY statsChanged)
    Q_PROPERTY(int videoWidth READ videoWidth NOTIFY videoGeometryChanged)
    Q_PROPERTY(int videoHeight READ videoHeight NOTIFY videoGeometryChanged)
    Q_PROPERTY(bool audioActive READ audioActive NOTIFY audioActiveChanged)
    Q_PROPERTY(int audioPacketCount READ audioPacketCount NOTIFY audioStatsChanged)
    Q_PROPERTY(QVideoSink *videoSink READ videoSink WRITE setVideoSink NOTIFY videoSinkChanged)

public:
    explicit CarplayController(QObject *parent = nullptr);
    ~CarplayController() override;

    QString status() const;
    bool dongleReady() const;
    bool streaming() const;
    bool receivingVideo() const;
    int frameCount() const;
    double decodedFps() const;
    int videoWidth() const;
    int videoHeight() const;
    bool audioActive() const;
    int audioPacketCount() const;
    QVideoSink *videoSink() const;
    void setVideoSink(QVideoSink *sink);

    Q_INVOKABLE void verifyHardware();
    Q_INVOKABLE void startStream(int width, int height, int fps);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void playCapture(const QString &captureDirectory, int fps);
    Q_INVOKABLE void touchDown(double x, double y);
    Q_INVOKABLE void touchMove(double x, double y);
    Q_INVOKABLE void touchUp(double x, double y);

signals:
    void statusChanged();
    void dongleReadyChanged();
    void streamingChanged();
    void receivingVideoChanged();
    void statsChanged();
    void videoGeometryChanged();
    void audioActiveChanged();
    void audioStatsChanged();
    void videoSinkChanged();

private:
    struct EncodedFrame {
        std::vector<uint8_t> bytes;
        int width = 0;
        int height = 0;
    };

    void setStatus(const QString &status);
    void setDongleReady(bool ready);
    void setStreaming(bool streaming);
    void setReceivingVideo(bool receiving);
    void setVideoGeometry(int width, int height);
    void setAudioActive(bool active);
    void resetStats();
    void startDecoder(int fps);
    void stopDecoder();
    void enqueueEncodedFrame(std::vector<uint8_t> bytes, int width, int height);
    void decoderLoop(int fps);
    void presentFrame(const QVideoFrame &frame);
    void captureLoop(QString captureDirectory, int fps);
    void handleTransportMessage(CarplayProtocol::Header header, const QByteArray &payload);
    void handleVideoPacket(const CarplayProtocol::VideoPacket &packet);
    void handleAudioPacket(const CarplayProtocol::AudioPacket &packet);
    void handleDongleCommand(int command);
    void writeAudio(int decodeType, int audioType, float volume, const QByteArray &pcm);
    void stopAudio();
    void startMicrophone();
    void stopMicrophone();
    void sendMicrophoneData(const QByteArray &pcm);
    void sendTouch(double x, double y, CarplayProtocol::TouchAction action);
    void scheduleFramePresentation(const QVideoFrame &frame);

    mutable std::mutex m_stateMutex;
    QString m_status = "Idle";
    bool m_dongleReady = false;
    bool m_streaming = false;
    bool m_receivingVideo = false;
    int m_frameCount = 0;
    double m_decodedFps = 0.0;
    int m_videoWidth = 0;
    int m_videoHeight = 0;
    bool m_audioActive = false;
    int m_audioPacketCount = 0;
    QPointer<QVideoSink> m_videoSink;

    struct AudioStream {
        QAudioSink *sink = nullptr;
        PcmRingBuffer *buffer = nullptr;
    };
    QHash<QString, AudioStream> m_audioStreams;
    QAudioSource *m_audioSource = nullptr;
    QPointer<QIODevice> m_audioInputDevice;

    std::mutex m_queueMutex;
    std::condition_variable m_queueCondition;
    std::deque<EncodedFrame> m_decodeQueue;
    std::atomic<bool> m_decoderRunning{false};
    std::thread m_decoderThread;
    std::mutex m_presentMutex;
    QVideoFrame m_pendingFrame;
    bool m_presentationScheduled = false;

    std::atomic<bool> m_captureRunning{false};
    std::thread m_captureThread;

    UsbDongleTransport *m_transport = nullptr;
};
