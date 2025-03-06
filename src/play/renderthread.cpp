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
    , m_currentVideoPts(0)
    , m_frameCount(0)
    , m_audioDevice(0)
    , m_syncData(nullptr)
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

void RenderThread::setSyncData(std::shared_ptr<SyncData> data)
{
    m_syncData = data;
}

void RenderThread::setVideoFrameQueue(AVFrameQueue *queue)
{
    m_videoFrameQueue = queue;
}

void RenderThread::setVideoWidget(SDLWidget *widget)
{
    m_videoWidget = widget;
}

bool RenderThread::initializeVideoRenderer()
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

    m_videoInitialized = true;
    qInfo() << "视频渲染器初始化成功";
    return true;
}

void RenderThread::closeRenderer()
{
    qDebug() << "关闭渲染器，准备释放资源...";

    // 清理视频渲染器资源
    m_videoInitialized = false;

    m_frameCount = 0;
    qDebug() << "渲染器资源释放完毕";
}

int64_t RenderThread::getCurrentVideoPts() const
{
    return m_currentVideoPts;
}

void RenderThread::onSyncEvent(int64_t masterClock, double videoDelay, double audioDelay)
{
    // 不仅记录日志，同时基于同步信息更新内部状态
    static int syncEventCounter = 0;
    syncEventCounter++;

    // 每10个事件记录一次日志，避免输出过多
    if (syncEventCounter % 10 == 0) {
        qDebug() << "同步事件: 主时钟=" << masterClock << "视频延迟=" << videoDelay
                 << "音频延迟=" << audioDelay;
    }

    // TODO: 在未来的版本中，可以在这里实现更高级的同步机制
    // 例如：根据同步状态动态调整视频渲染速率或音频播放缓冲区大小
}

// void RenderThread::process()
// {
//     AVFrame *videoFrame = nullptr;

//     // 优先处理pending帧
//     if (m_pendingFrame) {
//         videoFrame = m_pendingFrame;
//         m_pendingFrame = nullptr;
//     }
//     // 从队列获取新帧（如果可用）
//     else if (m_videoInitialized && m_videoFrameQueue && !m_videoFrameQueue->isEmpty()) {
//         videoFrame = m_videoFrameQueue->dequeueNoWait();
//     }

//     bool shouldRender = false;
//     bool shouldFree = true;

//     if (videoFrame) {
//         const int64_t masterTime = m_syncData->getMasterClock(); // 确保单位与pts一致
//         const int64_t diff = videoFrame->pts - masterTime;

//         if (diff < SYNC_THRESHOLD_MIN) {
//             // 帧已过时，直接丢弃
//             qDebug() << "丢弃过时帧，差异值：" << diff / 1000 << "ms";
//             shouldFree = true;
//             shouldRender = false;
//         } else if (diff > SYNC_THRESHOLD_MAX) {
//             // 帧过早，暂存并等待下次处理
//             m_pendingFrame = videoFrame;
//             shouldFree = false;
//             shouldRender = false;
//             qDebug() << "暂存超前帧，差异值：" << diff / 1000 << "ms";
//         } else {
//             // 在同步窗口内，立即渲染
//             shouldRender = true;
//             shouldFree = true;
//         }

//         if (shouldRender) {
//             renderVideoFrame(videoFrame);
//         }
//         if (shouldFree) {
//             av_frame_free(&videoFrame);
//         }
//     }

//     // 结束条件判断（需队列完成且无pending帧）
//     if (m_videoFrameQueue && m_videoFrameQueue->isFinished() && m_videoFrameQueue->isEmpty()
//         && !m_pendingFrame) {
//         pauseProcess();
//     }

//     // 动态调整渲染间隔
//     int sleepDuration = 1; // 默认1ms
//     if (m_pendingFrame) {
//         // 有pending帧时减少等待
//         sleepDuration = 0;
//     } else if (!m_videoFrameQueue || m_videoFrameQueue->isEmpty()) {
//         // 队列空闲时增加等待以降低CPU占用
//         sleepDuration = 10;
//     }
//     msleep(sleepDuration);
// }

void RenderThread::process()
{
    AVFrame *videoFrame = nullptr;

    // 获取视频帧
    if (m_videoInitialized && m_videoFrameQueue && !m_videoFrameQueue->isEmpty())
        videoFrame = m_videoFrameQueue->dequeueNoWait();

    bool renderFrame = true;

    // 处理视频帧
    if (videoFrame) {
        int64_t masterTime = m_syncData->getMasterClock(); // 获取主时钟
        auto    diff = videoFrame->pts - masterTime;
        if (diff > 1000)
            msleep(20);
        else if (diff > 500) {
            msleep(10);
            // renderFrame = false;
        } else if (diff < -100)
            renderFrame = false;
        if (renderFrame)
            renderVideoFrame(videoFrame);
        av_frame_free(&videoFrame);
    }

    // 检查是否都已结束
    if (m_videoFrameQueue && m_videoFrameQueue->isFinished() && m_videoFrameQueue->isEmpty()) {
        // 暂停线程，等待重新开始
        pauseProcess();
    }

    // 控制渲染速率，避免CPU占用过高
    if (renderFrame)
        msleep(1);
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
        m_frameCount++;

        // 更新视频时间戳到 SyncData
        if (frame->pts != AV_NOPTS_VALUE) {
            // 已经在process()方法中更新时间戳，此处不需要重复设置
            // m_currentVideoPts = frame->pts;
            if (m_syncData) {
                m_syncData->setVideoData(frame->pts,
                                         /*frame->time_base.num*/ 1,
                                         /*frame->time_base.den*/ 60);
            }
            // emit frameRendered(m_currentVideoPts);
        }

        // 每100帧输出一次日志
        // if (m_frameCount % 100 == 0) {
        //     qInfo() << "已渲染" << m_frameCount << "帧";
        // }

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
