#include "videodecodethread.h"
#include "avframequeue.h"
#include "avpacketqueue.h"

#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
}

VideoDecodeThread::VideoDecodeThread(QObject *parent)
    : ThreadBase(parent)
    , m_codecContext(nullptr)
    , m_packetQueue(nullptr)
    , m_frameQueue(new AVFrameQueue(1000))
    , m_streamIndex(-1)
{}

VideoDecodeThread::~VideoDecodeThread()
{
    stopProcess();
    wait();
    closeDecoder();
}

bool VideoDecodeThread::initialize()
{
    // 视频解码线程初始化
    return true;
}

void VideoDecodeThread::setPacketQueue(AVPacketQueue *queue)
{
    m_packetQueue = queue;
}

AVFrameQueue *VideoDecodeThread::getFrameQueue() const
{
    return m_frameQueue.get();
}

bool VideoDecodeThread::openDecoder(int streamIndex, AVCodecParameters *codecParams)
{
    if (!codecParams) {
        qWarning() << "无效的编解码参数";
        return false;
    }

    // 关闭之前的解码器
    closeDecoder();

    m_streamIndex = streamIndex;

    // 查找解码器
    const AVCodec *decoder = avcodec_find_decoder(codecParams->codec_id);
    if (!decoder) {
        qWarning() << "找不到解码器，编解码器ID:" << codecParams->codec_id;
        return false;
    }

    // 创建解码器上下文
    m_codecContext = avcodec_alloc_context3(decoder);
    if (!m_codecContext) {
        qWarning() << "无法分配解码器上下文";
        return false;
    }

    // 将解码参数复制到上下文
    if (avcodec_parameters_to_context(m_codecContext, codecParams) < 0) {
        qWarning() << "无法复制编解码器参数";
        closeDecoder();
        return false;
    }

    // 设置解码器线程数
    m_codecContext->thread_type = FF_THREAD_FRAME; // 明确使用帧级多线程
    m_codecContext->thread_count = 4;              // 使用多线程解码

    // 打开解码器
    if (avcodec_open2(m_codecContext, decoder, nullptr) < 0) {
        qWarning() << "无法打开解码器";
        closeDecoder();
        return false;
    }

    // 准备帧队列
    m_frameQueue->clear();

    qInfo() << "视频解码器已成功打开, 编解码器:" << decoder->name;
    return true;
}

void VideoDecodeThread::closeDecoder()
{
    // 清空帧队列
    if (m_frameQueue) {
        m_frameQueue->setFinished(true);
        m_frameQueue->clear();
    }

    // 释放解码器上下文
    if (m_codecContext) {
        avcodec_free_context(&m_codecContext);
        m_codecContext = nullptr;
    }

    m_streamIndex = -1;
}

void VideoDecodeThread::flush()
{
    QMutexLocker locker(&m_flushMutex);
    if (m_codecContext) {
        // 刷新解码器
        avcodec_flush_buffers(m_codecContext);
    }

    // 清空帧队列
    if (m_frameQueue) {
        m_frameQueue->clear();
    }
}

void VideoDecodeThread::process()
{
    if (!m_codecContext || !m_packetQueue || !m_frameQueue) {
        msleep(10);
        return;
    }

    // 如果帧队列已满，等待
    if (m_frameQueue->isFull()) {
        msleep(10);
        return;
    }

    QMutexLocker locker(&m_flushMutex);
    // 从包队列中获取一个包
    AVPacket *packet = m_packetQueue->dequeue(10);

    if (packet) {
        // 解码包
        if (!decodePacket(packet)) {
            qWarning() << "解码包失败";
        }

        // 释放包
        av_packet_free(&packet);
    } else {
        // 检查队列是否已经结束且为空
        if (m_packetQueue->isFinished() && m_packetQueue->isEmpty()) {
            // 发送空包来刷新解码器中的缓冲帧
            decodePacket(nullptr);

            // 设置帧队列为结束状态
            m_frameQueue->setFinished(true);

            // 发出解码完成信号
            emit decodeFinished();

            // 暂停线程，等待重新开始
            pauseProcess();
        }
    }
}

bool VideoDecodeThread::decodePacket(AVPacket *packet)
{
    if (!m_codecContext) {
        return false;
    }

    // 发送包到解码器
    int ret = avcodec_send_packet(m_codecContext, packet);
    if (ret < 0) {
        if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
            qWarning() << "发送包到解码器失败:" << errbuf;
            return false;
        }
    }

    // 从解码器接收帧
    while (ret >= 0) {
        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(m_codecContext, frame);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            // 需要更多输入包或到达流结束
            av_frame_free(&frame);
            break;
        } else if (ret < 0) {
            // 解码出错
            av_frame_free(&frame);
            char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
            qWarning() << "从解码器接收帧失败:" << errbuf;
            return false;
        }

        // 将解码后的帧放入帧队列
        if (!m_frameQueue->enqueue(frame)) {
            qWarning() << "将帧放入队列失败";
            av_frame_free(&frame);
            return false;
        }

        // 释放帧
        av_frame_free(&frame);
    }

    return true;
}

void VideoDecodeThread::cleanup()
{
    closeDecoder();
}
