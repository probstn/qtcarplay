#include "PcmRingBuffer.h"

#include <algorithm>
#include <cstring>

PcmRingBuffer::PcmRingBuffer(qsizetype capacityBytes, QObject *parent)
    : QIODevice(parent),
      m_buffer(std::max<qsizetype>(capacityBytes, 4096), Qt::Uninitialized)
{
    open(QIODevice::ReadWrite | QIODevice::Unbuffered);
}

qint64 PcmRingBuffer::bytesAvailable() const
{
    QMutexLocker lock(&m_mutex);
    return m_size + QIODevice::bytesAvailable();
}

qint64 PcmRingBuffer::readData(char *data, qint64 maxSize)
{
    QMutexLocker lock(&m_mutex);

    const qsizetype requested = static_cast<qsizetype>(maxSize);
    const qsizetype readable = std::min(requested, m_size);
    qsizetype copied = 0;

    while (copied < readable) {
        const qsizetype chunk = std::min(readable - copied, m_buffer.size() - m_readIndex);
        std::memcpy(data + copied, m_buffer.constData() + m_readIndex, static_cast<size_t>(chunk));
        m_readIndex = (m_readIndex + chunk) % m_buffer.size();
        m_size -= chunk;
        copied += chunk;
    }

    if (copied < requested)
        std::memset(data + copied, 0, static_cast<size_t>(requested - copied));

    return requested;
}

qint64 PcmRingBuffer::writeData(const char *data, qint64 maxSize)
{
    push(QByteArray(data, static_cast<qsizetype>(maxSize)));
    return maxSize;
}

void PcmRingBuffer::push(const QByteArray &data)
{
    QMutexLocker lock(&m_mutex);

    const char *cursor = data.constData();
    qsizetype remaining = data.size();

    if (remaining >= m_buffer.size()) {
        cursor += remaining - m_buffer.size();
        remaining = m_buffer.size();
        m_readIndex = 0;
        m_writeIndex = 0;
        m_size = 0;
    }

    const qsizetype overflow = std::max<qsizetype>(0, m_size + remaining - m_buffer.size());
    if (overflow > 0) {
        m_readIndex = (m_readIndex + overflow) % m_buffer.size();
        m_size -= overflow;
    }

    qsizetype copied = 0;
    while (copied < remaining) {
        const qsizetype chunk = std::min(remaining - copied, m_buffer.size() - m_writeIndex);
        std::memcpy(m_buffer.data() + m_writeIndex, cursor + copied, static_cast<size_t>(chunk));
        m_writeIndex = (m_writeIndex + chunk) % m_buffer.size();
        m_size += chunk;
        copied += chunk;
    }
}

void PcmRingBuffer::clear()
{
    QMutexLocker lock(&m_mutex);
    m_readIndex = 0;
    m_writeIndex = 0;
    m_size = 0;
}
