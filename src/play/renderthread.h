#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include "avsync.h"
#include "threadbase.h"

#include <SDL.h>
#include <memory>
#include <QMutex>
#include <QQueue>

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

class AVFrameQueue;
class SDLWidget;

/**
 * @brief 渲染线程类 - 负责将解码后的视频帧渲染到屏幕上
 */
class RenderThread : public ThreadBase
{
    Q_OBJECT
public:
    explicit RenderThread(QObject *parent = nullptr);
    ~RenderThread() override;

    // 初始化线程
    bool initialize() override;

    // 设置视频帧队列
    void setVideoFrameQueue(AVFrameQueue *queue);

    // 设置视频渲染控件
    void setVideoWidget(SDLWidget *widget);

    // 初始化视频渲染器
    bool initializeVideoRenderer(AVRational timebase);

    // 设置同步时钟
    void setSync(AVSync *sync);

    // 关闭渲染器
    void closeRenderer();

protected:
    // 线程处理函数
    void process() override;

private:
    // 渲染视频帧
    bool renderVideoFrame(AVFrame *frame);

    // 清理资源
    void cleanup();

private:
    AVFrameQueue *m_videoFrameQueue{nullptr};
    AVFrame      *m_currentRenderFrame{nullptr};

    SDLWidget *m_videoWidget{nullptr};

    AVSync    *m_avSync = nullptr;
    AVRational m_timebase;

    QMutex m_videoMutex;
    bool   m_videoInitialized{false};
};

#endif // RENDERTHREAD_H
