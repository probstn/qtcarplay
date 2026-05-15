#pragma once

#include <QByteArray>
#include <QIODevice>
#include <QMutex>

class PcmRingBuffer : public QIODevice
{
    Q_OBJECT

public:
    explicit PcmRingBuffer(qsizetype capacityBytes, QObject *parent = nullptr);

    bool isSequential() const override { return true; }
    qint64 bytesAvailable() const override;
    qint64 readData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *data, qint64 maxSize) override;

    void push(const QByteArray &data);
    void clear();

private:
    mutable QMutex m_mutex;
    QByteArray m_buffer;
    qsizetype m_readIndex = 0;
    qsizetype m_writeIndex = 0;
    qsizetype m_size = 0;
};
