#include "UsbDongleTransport.h"

#include <libusb.h>

#include <QDeadlineTimer>

#include <array>
#include <chrono>
#include <stdexcept>

namespace {

constexpr int ResetWaitMs = 3000;
constexpr int ResetProbeIntervalMs = 250;
constexpr int ResetProbeAttempts = 24;
constexpr int PairTimeoutMs = 15000;
constexpr qsizetype MaxPayloadSize = 1024 * 1024;
constexpr std::array<quint16, 2> ProductIds = {
    CarplayProtocol::ProductIdCpc200Ccpa,
    CarplayProtocol::ProductIdCpc200Ccpm,
};

QString usbError(int code)
{
    return QString::fromUtf8(libusb_error_name(code));
}

void requireUsb(int code, const QString &operation)
{
    if (code != LIBUSB_SUCCESS)
        throw std::runtime_error(QString("%1 failed: %2").arg(operation, usbError(code)).toStdString());
}

} // namespace

UsbDongleTransport::UsbDongleTransport(QObject *parent)
    : QThread(parent)
{
}

UsbDongleTransport::~UsbDongleTransport()
{
    stopTransport();
}

void UsbDongleTransport::startTransport(const CarplayProtocol::DongleConfig &config)
{
    if (isRunning())
        stopTransport();

    m_config = config;
    m_stopping = false;
    start(QThread::TimeCriticalPriority);
}

void UsbDongleTransport::stopTransport()
{
    m_stopping = true;
    if (isRunning())
        wait(4000);
    closeUsb();
}

bool UsbDongleTransport::sendMessage(const QByteArray &message)
{
    QMutexLocker lock(&m_usbMutex);
    if (!m_handle || m_endpointOut == 0)
        return false;

    int transferred = 0;
    const int result = libusb_bulk_transfer(m_handle,
                                            m_endpointOut,
                                            reinterpret_cast<unsigned char *>(const_cast<char *>(message.constData())),
                                            static_cast<int>(message.size()),
                                            &transferred,
                                            100);
    if (result != LIBUSB_SUCCESS || transferred != message.size()) {
        emit statusChanged(QString("USB write failed: %1 (%2/%3)")
                               .arg(usbError(result))
                               .arg(transferred)
                               .arg(message.size()));
        return false;
    }

    return true;
}

void UsbDongleTransport::run()
{
    try {
        initializeUsb();
        emit dongleReadyChanged(true);
        sendStartup();
        pollLoop();
    } catch (const std::exception &error) {
        emit transportFailed(QString::fromUtf8(error.what()));
    }

    emit dongleReadyChanged(false);
    closeUsb();
}

void UsbDongleTransport::initializeUsb()
{
    emit statusChanged(QStringLiteral("Opening CarPlay USB dongle"));

    requireUsb(libusb_init(&m_context), QStringLiteral("libusb_init"));
    if (!openKnownDevice())
        throw std::runtime_error("CarPlay dongle not found");

    emit statusChanged(QStringLiteral("Resetting dongle"));
    requireUsb(libusb_reset_device(m_handle), QStringLiteral("libusb_reset_device"));
    libusb_close(m_handle);
    m_handle = nullptr;

    for (int attempt = 0; attempt < ResetProbeAttempts && !m_stopping; ++attempt) {
        msleep(attempt == 0 ? ResetWaitMs : ResetProbeIntervalMs);
        if (openKnownDevice())
            break;
    }
    if (!m_handle)
        throw std::runtime_error("CarPlay dongle did not reappear after reset");

    const int configurationResult = libusb_set_configuration(m_handle, 1);
    if (configurationResult != LIBUSB_SUCCESS && configurationResult != LIBUSB_ERROR_BUSY) {
        emit statusChanged(QString("USB configuration warning: %1").arg(usbError(configurationResult)));
    }

    if (libusb_kernel_driver_active(m_handle, 0) == 1)
        libusb_detach_kernel_driver(m_handle, 0);

    requireUsb(libusb_claim_interface(m_handle, 0), QStringLiteral("libusb_claim_interface"));
    findEndpoints();

    emit statusChanged(QString("USB ready: IN 0x%1 OUT 0x%2")
                           .arg(m_endpointIn, 2, 16, QLatin1Char('0'))
                           .arg(m_endpointOut, 2, 16, QLatin1Char('0')));
}

bool UsbDongleTransport::openKnownDevice()
{
    for (const quint16 productId : ProductIds) {
        m_handle = libusb_open_device_with_vid_pid(m_context, CarplayProtocol::VendorId, productId);
        if (m_handle) {
            emit statusChanged(QString("Found CarPlay dongle PID 0x%1")
                                   .arg(productId, 4, 16, QLatin1Char('0')));
            return true;
        }
    }
    return false;
}

void UsbDongleTransport::closeUsb()
{
    QMutexLocker lock(&m_usbMutex);
    if (m_handle) {
        libusb_release_interface(m_handle, 0);
        libusb_close(m_handle);
        m_handle = nullptr;
    }
    if (m_context) {
        libusb_exit(m_context);
        m_context = nullptr;
    }
    m_endpointIn = 0;
    m_endpointOut = 0;
}

void UsbDongleTransport::findEndpoints()
{
    libusb_device *device = libusb_get_device(m_handle);
    libusb_config_descriptor *config = nullptr;
    requireUsb(libusb_get_active_config_descriptor(device, &config), QStringLiteral("libusb_get_active_config_descriptor"));

    if (config->bNumInterfaces == 0 || config->interface[0].num_altsetting == 0) {
        libusb_free_config_descriptor(config);
        throw std::runtime_error("No USB interface 0 altsetting");
    }

    const libusb_interface_descriptor &interface = config->interface[0].altsetting[0];
    for (int i = 0; i < interface.bNumEndpoints; ++i) {
        const libusb_endpoint_descriptor &endpoint = interface.endpoint[i];
        if ((endpoint.bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN)
            m_endpointIn = endpoint.bEndpointAddress;
        else if ((endpoint.bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT)
            m_endpointOut = endpoint.bEndpointAddress;
    }

    libusb_free_config_descriptor(config);

    if (!m_endpointIn || !m_endpointOut)
        throw std::runtime_error("USB bulk endpoints not found");
}

bool UsbDongleTransport::readExact(char *data, qsizetype size, unsigned int timeoutMs)
{
    QElapsedTimer timer;
    timer.start();
    const qint64 overallTimeoutMs = std::max<unsigned int>(timeoutMs * 5, 250);
    qsizetype offset = 0;
    while (!m_stopping && offset < size) {
        int transferred = 0;
        const int chunk = static_cast<int>(std::min<qsizetype>(size - offset, 256 * 1024));
        const int result = libusb_bulk_transfer(m_handle,
                                                m_endpointIn,
                                                reinterpret_cast<unsigned char *>(data + offset),
                                                chunk,
                                                &transferred,
                                                timeoutMs);

        if (result == LIBUSB_ERROR_TIMEOUT && transferred == 0) {
            if (offset == 0)
                return false;
            if (timer.elapsed() > overallTimeoutMs)
                throw std::runtime_error("Timed out during partial USB payload read");
            continue;
        }
        if (result != LIBUSB_SUCCESS && result != LIBUSB_ERROR_TIMEOUT)
            throw std::runtime_error(QString("USB read failed: %1").arg(usbError(result)).toStdString());

        offset += transferred;
    }

    return offset == size;
}

bool UsbDongleTransport::readNextHeader(CarplayProtocol::Header &header)
{
    QByteArray window(CarplayProtocol::HeaderSize, Qt::Uninitialized);
    if (!readExact(window.data(), window.size(), 50))
        return false;

    while (!m_stopping) {
        const auto parsed = CarplayProtocol::parseHeader(window);
        if (parsed) {
            header = *parsed;
            return true;
        }

        window.remove(0, 1);
        char next = 0;
        if (!readExact(&next, 1, 50))
            return false;
        window.append(next);
    }

    return false;
}

void UsbDongleTransport::sendStartup()
{
    emit statusChanged(QStringLiteral("Configuring dongle"));
    for (const QByteArray &message : CarplayProtocol::makeStartupMessages(m_config))
        sendMessage(message);

    msleep(1000);
    sendMessage(CarplayProtocol::makeCommand(CarplayProtocol::Command::WifiConnect));
    emit statusChanged(QStringLiteral("Dongle configured; waiting for stream"));
}

void UsbDongleTransport::pollLoop()
{
    QByteArray payload;
    payload.reserve(256 * 1024);

    QElapsedTimer timer;
    timer.start();
    const qint64 startMs = timer.elapsed();
    qint64 lastHeartbeatMs = timer.elapsed();
    int consecutiveErrors = 0;
    bool phoneSeen = false;
    bool pairSent = false;

    while (!m_stopping) {
        try {
            sendHeartbeatIfDue(timer, lastHeartbeatMs);
            sendPairIfDue(timer, startMs, phoneSeen, pairSent);

            CarplayProtocol::Header header;
            if (!readNextHeader(header))
                continue;

            if (header.length > MaxPayloadSize)
                throw std::runtime_error("USB payload exceeds maximum size");

            payload.resize(static_cast<qsizetype>(header.length));
            if (header.length > 0 && !readExact(payload.data(), payload.size(), 1000))
                throw std::runtime_error("Timed out waiting for USB payload");

            consecutiveErrors = 0;
            if (header.type == CarplayProtocol::MessageType::Plugged
                || header.type == CarplayProtocol::MessageType::VideoData
                || header.type == CarplayProtocol::MessageType::AudioData) {
                phoneSeen = true;
            } else if (header.type == CarplayProtocol::MessageType::Unplugged) {
                phoneSeen = false;
                pairSent = false;
            }
            emit messageReceived(header, payload);
        } catch (const std::exception &error) {
            emit statusChanged(QString("USB read warning: %1").arg(error.what()));
            if (++consecutiveErrors >= 10)
                throw;
        }
    }
}

void UsbDongleTransport::sendHeartbeatIfDue(QElapsedTimer &timer, qint64 &lastHeartbeatMs)
{
    const qint64 now = timer.elapsed();
    if (now - lastHeartbeatMs < 2000)
        return;

    sendMessage(CarplayProtocol::makeHeartbeat());
    lastHeartbeatMs = now;
}

void UsbDongleTransport::sendPairIfDue(QElapsedTimer &timer, qint64 startMs, bool phoneSeen, bool &pairSent)
{
    if (pairSent || phoneSeen)
        return;

    if (timer.elapsed() - startMs < PairTimeoutMs)
        return;

    emit statusChanged(QStringLiteral("No phone session yet; requesting wireless pair/connect"));
    sendMessage(CarplayProtocol::makeCommand(CarplayProtocol::Command::WifiPair));
    sendMessage(CarplayProtocol::makeCommand(CarplayProtocol::Command::WifiConnect));
    pairSent = true;
}
