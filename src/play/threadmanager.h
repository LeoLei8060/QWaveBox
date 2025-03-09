#ifndef THREADMANAGER_H
#define THREADMANAGER_H

#include "avsync.h"
#include "constants.h"
#include "syncdata.h"

#include <memory>
#include <QMap>
#include <QObject>

class SDLWidget;
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
#ifdef ENABLE_LIVE_DANMU
        AUDIO_RENDER, // 音频渲染线程
        DANMAKU,      // 弹幕处理线程
        LIVE_STREAM   // 直播流接收线程
#else
        AUDIO_RENDER // 音频渲染线程
#endif
    };

    explicit ThreadManager(QObject *parent = nullptr);
    ~ThreadManager();

    // 打开媒体
    bool openMedia(const QString &path);

    // 停止播放
    void stopPlay();

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

    // 绑定视频渲染对象
    void setVideoRenderObj(SDLWidget *obj);

    // 获取特定线程实例
    DemuxThread       *getDemuxThread();
    VideoDecodeThread *getVideoDecodeThread();
    AudioDecodeThread *getAudioDecodeThread();
    RenderThread      *getRenderThread();
    AudioRenderThread *getAudioRenderThread();
#ifdef ENABLE_LIVE_DANMU
    DanmakuThread    *getDanmakuThread();
    LiveStreamThread *getLiveStreamThread();
#endif

    void setPlaybackSpeed(double speed);

    double getPlaybackSpeed() const;

    double getCurrentPlayProgress();

    int64_t getPlayDuration();

    void setVolume(int volume);

signals:
    // 播放状态变化信号
    void sigPlayStateChanged(PlayState);

    // 播放错误信号
    void sigPlayError(const QString &errorMsg);

    // 播放完成信号
    void sigPlayFinished();

private:
    bool resetThreadLinkage();

    inline bool isPlaying() { return m_playState == PlayState::PlayingState; }
    inline bool isPauseed() { return m_playState == PlayState::PausedState; }
    inline bool isStopped() { return m_playState == PlayState::StoppedState; }

private:
    // 存储所有线程的映射
    QMap<ThreadType, std::shared_ptr<ThreadBase>> m_threads;

    // 是否已初始化
    bool m_initialized{false};

    // 播放状态
    PlayState m_playState;

    // 同步时钟
    AVSync m_avSync;
};

#endif // THREADMANAGER_H
