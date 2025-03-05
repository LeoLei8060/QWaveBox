#ifndef THREADMANAGER_H
#define THREADMANAGER_H

#include "syncdata.h"

#include <memory>
#include <QMap>
#include <QObject>

class ThreadBase;
class DemuxThread;
class VideoDecodeThread;
class AudioDecodeThread;
class RenderThread;
class AudioRenderThread;
class SyncThread;
class DanmakuThread;
class LiveStreamThread;

/**
 * @brief 线程管理类 - 管理播放器中的所有线程
 */
class ThreadManager : public QObject
{
    Q_OBJECT
public:
    enum ThreadType {
        DEMUX,        // 解复用线程
        VIDEO_DECODE, // 视频解码线程
        AUDIO_DECODE, // 音频解码线程
        VIDEO_RENDER, // 视频渲染线程
        AUDIO_RENDER, // 音频渲染线程
#ifdef ENABLE_LIVE_DANMU
        SYNC,       // 同步线程
        DANMAKU,    // 弹幕处理线程
        LIVE_STREAM // 直播流接收线程
#else
        SYNC // 同步线程
#endif
    };

    explicit ThreadManager(QObject *parent = nullptr);
    ~ThreadManager();

    bool openMedia(const QString &path);

    // 初始化所有线程
    bool initializeThreads();

    // 开始所有线程
    bool startAllThreads();

    // 暂停所有线程
    void pauseAllThreads();

    // 恢复所有线程
    void resumeAllThreads();

    // 停止所有线程
    void stopAllThreads();

    // 获取指定线程
    ThreadBase *getThread(ThreadType type);

    // 获取特定线程实例
    DemuxThread       *getDemuxThread();
    VideoDecodeThread *getVideoDecodeThread();
    AudioDecodeThread *getAudioDecodeThread();
    RenderThread      *getRenderThread();
    AudioRenderThread *getAudioRenderThread();
    SyncThread        *getSyncThread();
#ifdef ENABLE_LIVE_DANMU
    DanmakuThread    *getDanmakuThread();
    LiveStreamThread *getLiveStreamThread();
#endif

    // 设置播放速度
    void setPlaybackSpeed(double speed);

    // 获取当前播放速度
    double getPlaybackSpeed() const;

    // 设置音频采样率
    void setAudioSampleRate(int sampleRate);

    // 获取当前音频采样率
    int getAudioSampleRate() const;

signals:
    // 播放状态变化信号
    void playStateChanged(bool isPlaying);

    // 播放错误信号
    void playError(const QString &errorMsg);

    // 播放完成信号
    void playFinished();

private:
    // 存储所有线程的映射
    QMap<ThreadType, std::shared_ptr<ThreadBase>> m_threads;

    // 是否已初始化
    bool m_initialized{false};

    // 是否正在播放
    bool m_isPlaying{false};

    // 共享数据
    std::shared_ptr<SyncData> m_syncData;
};

#endif // THREADMANAGER_H
