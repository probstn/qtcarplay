#pragma once

#include <QElapsedTimer>
#include <QVideoFrame>

#include <cstdint>
#include <memory>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}

class H264Decoder
{
public:
    explicit H264Decoder(int targetFps = 30);
    ~H264Decoder();

    H264Decoder(const H264Decoder &) = delete;
    H264Decoder &operator=(const H264Decoder &) = delete;

    std::vector<QVideoFrame> decode(const uint8_t *data, qsizetype size);
    void reset();

private:
    QVideoFrame makeVideoFrame(const AVFrame *frame);
    QVideoFrame makeYuv420Frame(const AVFrame *frame);
    QVideoFrame makeNv12Frame(const AVFrame *frame);
    QVideoFrame makeConvertedYuv420Frame(const AVFrame *frame);
    QVideoFrame makeHardwareFrame(const AVFrame *frame);
    static AVPixelFormat choosePixelFormat(AVCodecContext *context, const AVPixelFormat *formats);

    AVCodecContext *m_codecContext = nullptr;
    AVCodecParserContext *m_parser = nullptr;
    AVFrame *m_frame = nullptr;
    AVFrame *m_hwTransferFrame = nullptr;
    AVPacket *m_packet = nullptr;
    AVBufferRef *m_hwDeviceContext = nullptr;
    SwsContext *m_swsContext = nullptr;
    AVFrame *m_conversionFrame = nullptr;
    std::vector<uint8_t> m_conversionBuffer;

    int m_targetFps = 30;
    qint64 m_nextStartTimeUs = 0;
};
