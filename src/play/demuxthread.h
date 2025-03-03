#ifndef DEMUXTHREAD_H
#define DEMUXTHREAD_H

#include "threadbase.h"
#include <memory>
#include <QMutex>
#include <QString>
#include <queue>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

class AVPacketQueue;

/**
 * @brief 解复用线程类 - 负责从文件或网络流中读取媒体数据包
 */
class DemuxThread : public ThreadBase
{
    Q_OBJECT
public:
    explicit DemuxThread(QObject *parent = nullptr);
    ~DemuxThread() override;

    // 初始化线程
    bool initialize() override;

    // 打开媒体文件或URL
    bool openMedia(const QString &path);

    // 关闭当前媒体
    void closeMedia();

    // 获取视频包队列
    AVPacketQueue *videoPacketQueue() const;

    // 获取音频包队列
    AVPacketQueue *audioPacketQueue() const;

    // 获取视频编解码参数
    AVCodecParameters *videoCodecParameters() const;

    // 获取音频编解码参数
    AVCodecParameters *audioCodecParameters() const;

    // 获取媒体总时长（毫秒）
    int64_t getDuration() const;

    // 获取视频宽度
    int getVideoWidth() const;

    // 获取视频高度
    int getVideoHeight() const;

    // 获取视频帧率
    double getFrameRate() const;

    // 获取视频流索引
    int getVideoStreamIndex() const;

    // 获取音频流索引
    int getAudioStreamIndex() const;

    // 获取当前播放位置（毫秒）
    int64_t getCurrentPosition() const;

    // 设置当前播放位置（毫秒）
    bool seekTo(int64_t position);

signals:
    void sigDemuxFinished();  // 解复用完成信号
    void sigMediaInfoReady(); // 媒体信息已准备好

    // void sigSendVideoPacket(AVPacket *); // 发送视频包数据
    // void sigSendAudioPacket(AVPacket *); // 发送音频包数据

protected:
    // 线程处理函数
    void process() override;

private:
    // 清理资源
    void cleanup();

    // 读取一个包
    bool readPacket();

private:
    // 媒体相关成员
    AVFormatContext *m_formatContext{nullptr};
    int              m_videoStreamIndex{-1};
    int              m_audioStreamIndex{-1};

    // 包队列
    std::unique_ptr<AVPacketQueue> m_videoPacketQueue;
    std::unique_ptr<AVPacketQueue> m_audioPacketQueue;

    // 媒体信息
    int     m_videoWidth{0};
    int     m_videoHeight{0};
    double  m_frameRate{0.0};
    int64_t m_duration{0}; // 媒体总时长（毫秒）

    AVCodecParameters *m_videoCodecParam{nullptr};
    AVCodecParameters *m_audioCodecParam{nullptr};

    // 当前位置
    std::atomic<int64_t> m_currentPosition{0}; // 当前播放位置（毫秒）

    // 媒体路径
    QString m_mediaPath;

    // 是否已经到达文件末尾
    std::atomic<bool> m_isEof{false};

    // 互斥锁
    QMutex m_seekMutex;
};

#endif // DEMUXTHREAD_H
