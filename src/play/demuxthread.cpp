#include "demuxthread.h"
#include "avpacketqueue.h"

#include <QDebug>

extern "C" {
#include <libavutil/time.h>
}

DemuxThread::DemuxThread(QObject *parent)
    : ThreadBase(parent)
    , m_formatContext(nullptr)
    , m_videoStreamIndex(-1)
    , m_audioStreamIndex(-1)
    , m_videoPacketQueue(new AVPacketQueue(100))
    , m_audioPacketQueue(new AVPacketQueue(200))
    , m_videoWidth(0)
    , m_videoHeight(0)
    , m_frameRate(0.0)
    , m_duration(0)
    , m_currentPosition(0)
    , m_isEof(false)
{}

DemuxThread::~DemuxThread()
{
    stopProcess();
    wait();
    closeMedia();
}

bool DemuxThread::initialize()
{
    // 初始化FFmpeg全局设置，只需要调用一次
    // 这个函数在新版FFmpeg中已不再需要，但为了兼容性仍保留
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
    av_register_all();
#endif

    // 初始化网络相关组件，以便可以打开网络流
    avformat_network_init();

    return true;
}

bool DemuxThread::openMedia(const QString &path)
{
    QMutexLocker locker(&m_mutex);

    // 先关闭之前的媒体
    closeMedia();

    m_mediaPath = path;
    m_isEof = false;
    m_currentPosition = 0;

    // 打开输入文件，并读取头部
    int ret = avformat_open_input(&m_formatContext,
                                  m_mediaPath.toUtf8().constData(),
                                  nullptr,
                                  nullptr);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        qWarning() << "无法打开媒体文件：" << path << "，错误: " << errbuf;
        return false;
    }

    // 检索流信息
    if (avformat_find_stream_info(m_formatContext, nullptr) < 0) {
        qWarning() << "无法找到流信息";
        closeMedia();
        return false;
    }

    // 查找第一个视频流和音频流
    m_videoStreamIndex = -1;
    m_audioStreamIndex = -1;

    for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
        AVStream *stream = m_formatContext->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && m_videoStreamIndex < 0) {
            m_videoStreamIndex = i;
            m_videoWidth = stream->codecpar->width;
            m_videoHeight = stream->codecpar->height;

            // 计算帧率
            if (stream->avg_frame_rate.num != 0 && stream->avg_frame_rate.den != 0) {
                m_frameRate = av_q2d(stream->avg_frame_rate);
            } else if (stream->r_frame_rate.num != 0 && stream->r_frame_rate.den != 0) {
                m_frameRate = av_q2d(stream->r_frame_rate);
            }
        } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && m_audioStreamIndex < 0) {
            m_audioStreamIndex = i;
        }
    }

    if (m_videoStreamIndex < 0 && m_audioStreamIndex < 0) {
        qWarning() << "未找到可播放的音视频流";
        closeMedia();
        return false;
    }

    // 获取媒体总时长（微秒转毫秒）
    if (m_formatContext->duration != AV_NOPTS_VALUE) {
        m_duration = m_formatContext->duration / 1000;
    }

    // 获取音视频编解码参数
    if (m_formatContext && m_videoStreamIndex >= 0)
        m_videoCodecParam = m_formatContext->streams[m_videoStreamIndex]->codecpar;
    if (m_formatContext && m_audioStreamIndex >= 0)
        m_audioCodecParam = m_formatContext->streams[m_audioStreamIndex]->codecpar;

    // 清空并准备包队列
    m_videoPacketQueue->clear();
    m_audioPacketQueue->clear();

    qInfo() << "媒体已成功打开：" << path;
    qInfo() << "视频流索引:" << m_videoStreamIndex << "分辨率:" << m_videoWidth << "x"
            << m_videoHeight << "帧率:" << m_frameRate;
    qInfo() << "音频流索引:" << m_audioStreamIndex;
    qInfo() << "时长(ms):" << m_duration;

    emit sigMediaInfoReady();

    return true;
}

void DemuxThread::closeMedia()
{
    // 停止队列
    if (m_videoPacketQueue) {
        m_videoPacketQueue->setFinished(true);
        m_videoPacketQueue->clear();
    }

    if (m_audioPacketQueue) {
        m_audioPacketQueue->setFinished(true);
        m_audioPacketQueue->clear();
    }

    // 关闭并释放格式上下文
    if (m_formatContext) {
        avformat_close_input(&m_formatContext);
        m_formatContext = nullptr;
    }

    // 重置流索引和媒体信息
    m_videoStreamIndex = -1;
    m_audioStreamIndex = -1;
    m_videoWidth = 0;
    m_videoHeight = 0;
    m_frameRate = 0.0;
    m_duration = 0;
    m_currentPosition = 0;
    m_isEof = false;
}

AVPacketQueue *DemuxThread::videoPacketQueue() const
{
    return m_videoPacketQueue.get();
}

AVPacketQueue *DemuxThread::audioPacketQueue() const
{
    return m_audioPacketQueue.get();
}

AVCodecParameters *DemuxThread::videoCodecParameters() const
{
    return m_videoCodecParam;
}

AVCodecParameters *DemuxThread::audioCodecParameters() const
{
    return m_audioCodecParam;
}

AVRational DemuxThread::videoTimebase() const
{
    if (m_videoStreamIndex != -1)
        return m_formatContext->streams[m_videoStreamIndex]->time_base;
    else
        return AVRational{0, 0};
}

AVRational DemuxThread::audioTimebase() const
{
    if (m_audioStreamIndex != -1)
        return m_formatContext->streams[m_audioStreamIndex]->time_base;
    else
        return AVRational{0, 0};
}

int64_t DemuxThread::getDuration() const
{
    return m_duration;
}

int DemuxThread::getVideoWidth() const
{
    return m_videoWidth;
}

int DemuxThread::getVideoHeight() const
{
    return m_videoHeight;
}

double DemuxThread::getFrameRate() const
{
    return m_frameRate;
}

int DemuxThread::getVideoStreamIndex() const
{
    return m_videoStreamIndex;
}

int DemuxThread::getAudioStreamIndex() const
{
    return m_audioStreamIndex;
}

int64_t DemuxThread::getCurrentPosition() const
{
    return m_currentPosition;
}

bool DemuxThread::seekTo(int64_t position)
{
    if (!m_formatContext || position < 0 || position > m_duration) {
        return false;
    }

    QMutexLocker locker(&m_seekMutex);

    // 将毫秒转换为微秒（FFmpeg内部时间基准）
    int64_t seekTarget = position * 1000;
    int     ret = avformat_seek_file(m_formatContext, -1, INT64_MIN, seekTarget, INT64_MAX, 0);

    if (ret < 0) {
        qWarning() << "Seek失败:" << position;
        return false;
    }

    // 清空包队列中的旧数据
    m_videoPacketQueue->clear();
    m_audioPacketQueue->clear();

    // 更新当前位置
    m_currentPosition = position;
    m_isEof = false;

    qInfo() << "Seek到位置:" << position << "ms";
    return true;
}

void DemuxThread::process()
{
    if (!m_formatContext) {
        msleep(10);
        return;
    }

    // 队列未满时继续读取包
    if (!m_videoPacketQueue->isFull() && !m_audioPacketQueue->isFull() && !m_isEof) {
        if (!readPacket()) {
            // 读取失败但不是因为EOF，短暂等待后重试
            if (!m_isEof) {
                msleep(10);
            }
        }
    } else {
        // 队列已满或已读取完毕，等待一段时间
        msleep(1);
    }

    // 如果已经到达文件末尾且队列为空，发出解复用完成信号
    if (m_isEof && m_videoPacketQueue->isEmpty() && m_audioPacketQueue->isEmpty()) {
        emit sigDemuxFinished();

        // 暂停线程，等待重新开始或其他操作
        pauseProcess();
    }
}

void DemuxThread::cleanup()
{
    closeMedia();
}

bool DemuxThread::readPacket()
{
    if (!m_formatContext) {
        return false;
    }

    // 检查是否需要暂停读取
    if (m_videoPacketQueue->isFull() || m_audioPacketQueue->isFull()) {
        return false;
    }

    // 避免在seek操作期间读取包
    QMutexLocker locker(&m_seekMutex);

    AVPacket *packet = av_packet_alloc();
    int       ret = av_read_frame(m_formatContext, packet);

    if (ret < 0) {
        av_packet_free(&packet);

        if (ret == AVERROR_EOF) {
            // 文件已读取完毕
            qInfo() << "已到达文件末尾";
            m_isEof = true;

            // 通知队列不会有更多数据
            m_videoPacketQueue->setFinished(true);
            m_audioPacketQueue->setFinished(true);
        } else {
            // 其他错误
            char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
            qWarning() << "读取媒体包时出错:" << errbuf;
        }

        return false;
    }

    // 将包放入对应的队列
    bool enqueued = false;

    if (packet->stream_index == m_videoStreamIndex) {
        enqueued = m_videoPacketQueue->enqueue(packet);
    } else if (packet->stream_index == m_audioStreamIndex) {
        enqueued = m_audioPacketQueue->enqueue(packet);
    }

    // qDebug() << __FUNCTION__ << "  " << packet->dts;

    // 释放原始包（队列中已经有了引用副本）
    av_packet_free(&packet);

    // 更新当前播放位置
    // if (m_formatContext->streams && m_videoStreamIndex >= 0) {
    //     AVStream *stream = m_formatContext->streams[m_videoStreamIndex];
    //     if (packet->pts != AV_NOPTS_VALUE) {
    //         m_currentPosition = packet->pts * av_q2d(stream->time_base) * 1000;
    //     }
    // }

    return enqueued;
}
