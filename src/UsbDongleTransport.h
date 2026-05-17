#pragma once

#include "CarplayProtocol.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QMutex>
#include <QThread>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

struct libusb_context;
struct libusb_device_handle;

class UsbDongleTransport : public QThread
{
    Q_OBJECT

public:
    explicit UsbDongleTransport(QObject *parent = nullptr);
    ~UsbDongleTransport() override;

    void startTransport(const CarplayProtocol::DongleConfig &config);
    void stopTransport();
    bool sendMessage(const QByteArray &message);

signals:
    void statusChanged(const QString &status);
    void dongleReadyChanged(bool ready);
    void messageReceived(CarplayProtocol::Header header, QByteArray payload);
    void transportFailed(const QString &reason);

protected:
    void run() override;

private:
    void initializeUsb();
    void closeUsb();
    bool openKnownDevice();
    void findEndpoints();
    bool readExact(char *data, qsizetype size, unsigned int timeoutMs);
    bool readNextHeader(CarplayProtocol::Header &header);
    void sendStartup();
    void pollLoop();
    void sendHeartbeatIfDue(QElapsedTimer &timer, qint64 &lastHeartbeatMs);
    void sendPairIfDue(QElapsedTimer &timer, qint64 startMs, bool phoneSeen, bool &pairSent);
    void startWriter();
    void stopWriter();
    void writerLoop();
    bool writeMessageNow(const QByteArray &message, int timeoutMs);

    CarplayProtocol::DongleConfig m_config;
    libusb_context *m_context = nullptr;
    libusb_device_handle *m_handle = nullptr;
    uint8_t m_endpointIn = 0;
    uint8_t m_endpointOut = 0;
    QMutex m_usbMutex;
    std::atomic_bool m_stopping{false};
    std::atomic_bool m_writerStopping{false};
    std::mutex m_writeMutex;
    std::condition_variable m_writeCondition;
    std::deque<QByteArray> m_writeQueue;
    std::thread m_writerThread;
};
