#include "renderthread.h"
#include "../gui/sdlwidget.h"
#include "avframequeue.h"

#include <QDebug>
#include <QElapsedTimer>

extern "C" {
#include <SDL2/SDL.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>
}

RenderThread::RenderThread(QObject *parent)
    : ThreadBase(parent)
    , m_videoFrameQueue(nullptr)
    , m_videoWidget(nullptr)
    , m_videoInitialized(false)
{}

RenderThread::~RenderThread()
{
    qDebug() << "RenderThread 析构函数开始执行";

    // 停止线程
    stopProcess();

    // 确保线程已经退出
    if (isRunning()) {
        qDebug() << "等待线程结束...";
        wait();
    }

    // 关闭渲染器并释放资源
    closeRenderer();

    qDebug() << "RenderThread 析构函数执行完毕";
}

bool RenderThread::initialize()
{
    // 渲染线程初始化
    return true;
}

void RenderThread::setVideoFrameQueue(AVFrameQueue *queue)
{
    m_videoFrameQueue = queue;
}

void RenderThread::setVideoWidget(SDLWidget *widget)
{
    m_videoWidget = widget;
}

bool RenderThread::initializeVideoRenderer(AVSync *sync, AVRational timebase)
{
    if (!m_videoWidget) {
        qWarning() << "无效的视频渲染控件";
        return false;
    }

    // 初始化SDL渲染器
    if (!m_videoWidget->initializeSDL()) {
        qWarning() << "初始化SDL渲染器失败";
        return false;
    }

    m_avSync = sync;
    m_timebase = timebase;

    m_videoInitialized = true;
    qInfo() << "视频渲染器初始化成功";
    return true;
}

void RenderThread::closeRenderer()
{
    qDebug() << "关闭渲染器，准备释放资源...";

    // 清理视频渲染器资源
    m_videoInitialized = false;

    qDebug() << "渲染器资源释放完毕";
}

void RenderThread::process()
{
    AVFrame *videoFrame = nullptr;

    // 获取视频帧
    if (m_videoInitialized && m_videoFrameQueue && !m_videoFrameQueue->isEmpty())
        videoFrame = m_videoFrameQueue->front();

    double sleepTime = 0;

    // 处理视频帧
    if (videoFrame) {
        double tm = videoFrame->pts * av_q2d(m_timebase);
        double diff = tm - m_avSync->getClock();
        if (diff > 0) {
            sleepTime = FFMIN(sleepTime, diff);
            return;
        }
        renderVideoFrame(videoFrame);
        m_videoFrameQueue->pop();
        av_frame_free(&videoFrame);
    }

    // 检查是否都已结束
    if (m_videoFrameQueue && m_videoFrameQueue->isFinished() && m_videoFrameQueue->isEmpty()) {
        // 暂停线程，等待重新开始
        pauseProcess();
    }

    // 控制渲染速率，避免CPU占用过高
    msleep(sleepTime * 1000);
}

bool RenderThread::renderVideoFrame(AVFrame *frame)
{
    if (!m_videoWidget || !frame) {
        return false;
    }

    QMutexLocker locker(&m_videoMutex);

    try {
        // 渲染帧
        m_videoWidget->renderFrame(frame);

        return true;
    } catch (const std::exception &e) {
        qWarning() << "渲染视频帧出错:" << e.what();
        return false;
    }
}

void RenderThread::cleanup()
{
    closeRenderer();
}
