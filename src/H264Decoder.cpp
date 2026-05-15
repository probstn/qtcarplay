#include "H264Decoder.h"

#include <QDebug>
#include <QSize>
#include <QVideoFrameFormat>

extern "C" {
#include <libavutil/hwcontext.h>
#include <libavutil/imgutils.h>
}

#include <algorithm>
#include <stdexcept>

namespace {

void copyPlane(const uint8_t *src,
               int srcStride,
               uchar *dst,
               int dstStride,
               int bytesPerRow,
               int rows)
{
    for (int y = 0; y < rows; ++y)
        std::copy_n(src + y * srcStride, bytesPerRow, dst + y * dstStride);
}

} // namespace

H264Decoder::H264Decoder(int targetFps)
    : m_targetFps(std::max(1, targetFps))
{
    const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec)
        throw std::runtime_error("H264 decoder not found");

    m_codecContext = avcodec_alloc_context3(codec);
    m_parser = av_parser_init(codec->id);
    m_frame = av_frame_alloc();
    m_packet = av_packet_alloc();

    if (!m_codecContext || !m_parser || !m_frame || !m_packet)
        throw std::runtime_error("Failed to allocate H264 decoder");

    m_codecContext->opaque = this;
    m_codecContext->get_format = &H264Decoder::choosePixelFormat;
    m_codecContext->flags2 |= AV_CODEC_FLAG2_FAST;
    m_codecContext->flags |= AV_CODEC_FLAG_LOW_DELAY;
    m_codecContext->thread_count = 0;
    m_codecContext->thread_type = FF_THREAD_SLICE;

#if defined(__APPLE__)
    if (qEnvironmentVariableIntValue("QTCARPLAY_HW_DECODE") == 1
        && av_hwdevice_ctx_create(&m_hwDeviceContext, AV_HWDEVICE_TYPE_VIDEOTOOLBOX, nullptr, nullptr, 0) >= 0) {
        m_codecContext->hw_device_ctx = av_buffer_ref(m_hwDeviceContext);
    }
#endif

    if (avcodec_open2(m_codecContext, codec, nullptr) < 0)
        throw std::runtime_error("Could not open H264 decoder");
}

H264Decoder::~H264Decoder()
{
    if (m_swsContext)
        sws_freeContext(m_swsContext);
    if (m_conversionFrame)
        av_frame_free(&m_conversionFrame);
    if (m_hwTransferFrame)
        av_frame_free(&m_hwTransferFrame);
    if (m_hwDeviceContext)
        av_buffer_unref(&m_hwDeviceContext);
    if (m_packet)
        av_packet_free(&m_packet);
    if (m_frame)
        av_frame_free(&m_frame);
    if (m_parser)
        av_parser_close(m_parser);
    if (m_codecContext)
        avcodec_free_context(&m_codecContext);
}

void H264Decoder::reset()
{
    avcodec_flush_buffers(m_codecContext);
    m_nextStartTimeUs = 0;
}

std::vector<QVideoFrame> H264Decoder::decode(const uint8_t *data, qsizetype size)
{
    std::vector<QVideoFrame> frames;
    const uint8_t *cursor = data;
    int remaining = static_cast<int>(size);

    while (remaining > 0) {
        int consumed = av_parser_parse2(m_parser,
                                        m_codecContext,
                                        &m_packet->data,
                                        &m_packet->size,
                                        cursor,
                                        remaining,
                                        AV_NOPTS_VALUE,
                                        AV_NOPTS_VALUE,
                                        0);
        if (consumed < 0)
            throw std::runtime_error("H264 parser failed");

        cursor += consumed;
        remaining -= consumed;

        if (m_packet->size <= 0)
            continue;

        int sendResult = avcodec_send_packet(m_codecContext, m_packet);
        if (sendResult < 0) {
            qWarning() << "avcodec_send_packet failed" << sendResult;
            continue;
        }

        while (true) {
            int receiveResult = avcodec_receive_frame(m_codecContext, m_frame);
            if (receiveResult == AVERROR(EAGAIN) || receiveResult == AVERROR_EOF)
                break;
            if (receiveResult < 0)
                throw std::runtime_error("H264 decoder failed");

            QVideoFrame videoFrame = makeVideoFrame(m_frame);
            if (videoFrame.isValid()) {
                const qint64 durationUs = 1000000 / m_targetFps;
                videoFrame.setStartTime(m_nextStartTimeUs);
                videoFrame.setEndTime(m_nextStartTimeUs + durationUs);
                videoFrame.setStreamFrameRate(m_targetFps);
                m_nextStartTimeUs += durationUs;
                frames.push_back(std::move(videoFrame));
            }
            av_frame_unref(m_frame);
        }
    }

    return frames;
}

QVideoFrame H264Decoder::makeVideoFrame(const AVFrame *frame)
{
    switch (static_cast<AVPixelFormat>(frame->format)) {
#if defined(__APPLE__)
    case AV_PIX_FMT_VIDEOTOOLBOX:
        return makeHardwareFrame(frame);
#endif
    case AV_PIX_FMT_YUV420P:
    case AV_PIX_FMT_YUVJ420P:
        return makeYuv420Frame(frame);
    case AV_PIX_FMT_NV12:
        return makeNv12Frame(frame);
    default:
        return makeConvertedYuv420Frame(frame);
    }
}

QVideoFrame H264Decoder::makeHardwareFrame(const AVFrame *frame)
{
    if (!m_hwTransferFrame)
        m_hwTransferFrame = av_frame_alloc();
    if (!m_hwTransferFrame)
        return {};

    av_frame_unref(m_hwTransferFrame);
    if (av_hwframe_transfer_data(m_hwTransferFrame, frame, 0) < 0)
        return {};

    return makeVideoFrame(m_hwTransferFrame);
}

AVPixelFormat H264Decoder::choosePixelFormat(AVCodecContext *context, const AVPixelFormat *formats)
{
    auto *decoder = static_cast<H264Decoder *>(context->opaque);

    for (const AVPixelFormat *format = formats; *format != AV_PIX_FMT_NONE; ++format) {
#if defined(__APPLE__)
        if (*format == AV_PIX_FMT_VIDEOTOOLBOX && decoder && decoder->m_hwDeviceContext)
            return *format;
#endif
    }

    for (const AVPixelFormat *format = formats; *format != AV_PIX_FMT_NONE; ++format) {
        if (*format == AV_PIX_FMT_NV12 || *format == AV_PIX_FMT_YUV420P || *format == AV_PIX_FMT_YUVJ420P)
            return *format;
    }

    return formats[0];
}

QVideoFrame H264Decoder::makeYuv420Frame(const AVFrame *frame)
{
    QVideoFrameFormat format(QSize(frame->width, frame->height), QVideoFrameFormat::Format_YUV420P);
    format.setColorRange(frame->color_range == AVCOL_RANGE_JPEG ? QVideoFrameFormat::ColorRange_Full
                                                                 : QVideoFrameFormat::ColorRange_Video);
    format.setColorSpace(QVideoFrameFormat::ColorSpace_BT709);

    QVideoFrame videoFrame(format);
    if (!videoFrame.map(QVideoFrame::WriteOnly))
        return {};

    copyPlane(frame->data[0], frame->linesize[0], videoFrame.bits(0), videoFrame.bytesPerLine(0), frame->width, frame->height);
    copyPlane(frame->data[1], frame->linesize[1], videoFrame.bits(1), videoFrame.bytesPerLine(1), frame->width / 2, frame->height / 2);
    copyPlane(frame->data[2], frame->linesize[2], videoFrame.bits(2), videoFrame.bytesPerLine(2), frame->width / 2, frame->height / 2);

    videoFrame.unmap();
    return videoFrame;
}

QVideoFrame H264Decoder::makeNv12Frame(const AVFrame *frame)
{
    QVideoFrameFormat format(QSize(frame->width, frame->height), QVideoFrameFormat::Format_NV12);
    format.setColorRange(frame->color_range == AVCOL_RANGE_JPEG ? QVideoFrameFormat::ColorRange_Full
                                                                 : QVideoFrameFormat::ColorRange_Video);
    format.setColorSpace(QVideoFrameFormat::ColorSpace_BT709);

    QVideoFrame videoFrame(format);
    if (!videoFrame.map(QVideoFrame::WriteOnly))
        return {};

    copyPlane(frame->data[0], frame->linesize[0], videoFrame.bits(0), videoFrame.bytesPerLine(0), frame->width, frame->height);
    copyPlane(frame->data[1], frame->linesize[1], videoFrame.bits(1), videoFrame.bytesPerLine(1), frame->width, frame->height / 2);

    videoFrame.unmap();
    return videoFrame;
}

QVideoFrame H264Decoder::makeConvertedYuv420Frame(const AVFrame *frame)
{
    m_swsContext = sws_getCachedContext(m_swsContext,
                                        frame->width,
                                        frame->height,
                                        static_cast<AVPixelFormat>(frame->format),
                                        frame->width,
                                        frame->height,
                                        AV_PIX_FMT_YUV420P,
                                        SWS_FAST_BILINEAR,
                                        nullptr,
                                        nullptr,
                                        nullptr);
    if (!m_swsContext)
        return {};

    if (!m_conversionFrame)
        m_conversionFrame = av_frame_alloc();
    if (!m_conversionFrame)
        return {};

    const int requiredSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, frame->width, frame->height, 1);
    if (requiredSize <= 0)
        return {};

    if (static_cast<int>(m_conversionBuffer.size()) < requiredSize)
        m_conversionBuffer.resize(requiredSize);

    av_image_fill_arrays(m_conversionFrame->data,
                         m_conversionFrame->linesize,
                         m_conversionBuffer.data(),
                         AV_PIX_FMT_YUV420P,
                         frame->width,
                         frame->height,
                         1);
    m_conversionFrame->width = frame->width;
    m_conversionFrame->height = frame->height;
    m_conversionFrame->format = AV_PIX_FMT_YUV420P;

    sws_scale(m_swsContext,
              frame->data,
              frame->linesize,
              0,
              frame->height,
              m_conversionFrame->data,
              m_conversionFrame->linesize);

    return makeYuv420Frame(m_conversionFrame);
}
