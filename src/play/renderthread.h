#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include "syncdata.h"
#include "threadbase.h"

#include <SDL.h>
#include <memory>
#include <QMutex>

extern "C" {
#include <libavutil/frame.h>
}

class AVFrameQueue;
class SDLWidget;

/**
 * @brief 渲染线程类 - 负责将解码后的视频帧渲染到屏幕上，并播放音频
 */
class RenderThread : public ThreadBase
{
    Q_OBJECT
public:
    explicit RenderThread(QObject *parent = nullptr);
    ~RenderThread() override;

    // 初始化线程
    bool initialize() override;

    void setSyncData(std::shared_ptr<SyncData> data);

    // 设置视频帧队列
    void setVideoFrameQueue(AVFrameQueue *queue);

    // 设置音频帧队列
    void setAudioFrameQueue(AVFrameQueue *queue);

    // 设置视频渲染控件
    void setVideoWidget(SDLWidget *widget);

    // 初始化视频渲染器
    bool initializeVideoRenderer();

    // 初始化音频渲染器
    bool initializeAudioRenderer(int sampleRate, int channels, int64_t channelLayout);

    // 关闭渲染器
    void closeRenderer();

    // 获取当前视频时间戳
    int64_t getCurrentVideoPts() const;

    // 获取当前音频时间戳
    int64_t getCurrentAudioPts() const;

signals:
    // 渲染完成信号
    void renderFinished();

    // 帧渲染信号
    void frameRendered(int64_t pts);

protected:
    // 线程处理函数
    void process() override;

private:
    // 渲染视频帧
    bool renderVideoFrame(AVFrame *frame);

    // 渲染音频帧
    bool renderAudioFrame(AVFrame *frame);

    // 清理资源
    void cleanup();

private:
    // 帧队列
    AVFrameQueue *m_videoFrameQueue{nullptr};
    AVFrameQueue *m_audioFrameQueue{nullptr};

    // 视频渲染控件
    SDLWidget *m_videoWidget{nullptr};

    // 是否已初始化
    bool m_videoInitialized{false};
    bool m_audioInitialized{false};

    // 当前帧时间戳(毫秒)
    std::atomic<int64_t> m_currentVideoPts{0};
    std::atomic<int64_t> m_currentAudioPts{0};

    // 同步锁
    QMutex m_videoMutex;
    QMutex m_audioMutex;

    // 视频帧计数
    int               m_frameCount{0};
    SDL_AudioDeviceID m_audioDevice;

    std::shared_ptr<SyncData> m_syncData{nullptr};
};

#endif // RENDERTHREAD_H
