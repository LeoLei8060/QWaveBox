#include "threadmanager.h"
#include "audiodecodethread.h"
#include "audiorenderthread.h"
#include "demuxthread.h"
#include "renderthread.h"
#include "syncthread.h"
#include "threadbase.h"
#include "videodecodethread.h"

#include <QDebug>

ThreadManager::ThreadManager(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
    , m_isPlaying(false)
{}

ThreadManager::~ThreadManager()
{
    stopAllThreads();
}

bool ThreadManager::openMedia(const QString &path)
{
    auto demuxThd = getDemuxThread();
    auto bRet = demuxThd->openMedia(path);
    return bRet;
}

bool ThreadManager::initializeThreads()
{
    if (m_initialized) {
        return true;
    }

    // 创建所有线程实例
    try {
        // 解复用线程
        m_threads[DEMUX] = std::make_shared<DemuxThread>();

        // 解码线程
        m_threads[VIDEO_DECODE] = std::make_shared<VideoDecodeThread>();
        m_threads[AUDIO_DECODE] = std::make_shared<AudioDecodeThread>();

        // 渲染线程
        m_threads[VIDEO_RENDER] = std::make_shared<RenderThread>();
        m_threads[AUDIO_RENDER] = std::make_shared<AudioRenderThread>();

        // 同步线程
        // m_threads[SYNC] = std::make_shared<SyncThread>();
#ifdef ENABLE_LIVE_DANMU
        // 弹幕线程
        m_threads[DANMAKU] = std::make_shared<DanmakuThread>();

        // 直播流线程
        m_threads[LIVE_STREAM] = std::make_shared<LiveStreamThread>();
#endif

        // 连接线程的错误信号
        for (auto it = m_threads.begin(); it != m_threads.end(); ++it) {
            connect(it.value().get(), &ThreadBase::threadError, this, [this](const QString &error) {
                emit playError(error);
                stopAllThreads();
            });
        }

        // 初始化所有线程
        for (auto it = m_threads.begin(); it != m_threads.end(); ++it) {
            if (!it.value()->initialize()) {
                qWarning() << "线程初始化失败：" << it.key();
                return false;
            }
        }

        m_initialized = true;
        return true;
    } catch (const std::exception &e) {
        qWarning() << "初始化线程时发生异常：" << e.what();
        return false;
    }
}

bool ThreadManager::initThreadLinkage(SDLWidget *renderWidget)
{
    bool bRet = false;
    // reset sync
    m_avSync.initClock();

    // demux -> decode (packetQueue)
    auto demuxThd = getDemuxThread();
    auto videoThd = getVideoDecodeThread();
    auto audioThd = getAudioDecodeThread();
    if (demuxThd && videoThd && audioThd) {
        videoThd->setPacketQueue(demuxThd->videoPacketQueue());
        videoThd->openDecoder(demuxThd->getVideoStreamIndex(), demuxThd->videoCodecParameters());
        audioThd->setPacketQueue(demuxThd->audioPacketQueue());
        audioThd->openDecoder(demuxThd->getAudioStreamIndex(), demuxThd->audioCodecParameters());
    } else
        return false;

    // video decode -> video render (frameQueue)
    auto vRenderThd = getRenderThread();
    if (videoThd && vRenderThd) {
        vRenderThd->setVideoFrameQueue(videoThd->getFrameQueue());
    } else
        return false;
    // audio decode -> audio render (frameQueue)
    auto aRenderThd = getAudioRenderThread();
    if (audioThd && aRenderThd) {
        aRenderThd->setAudioFrameQueue(audioThd->getFrameQueue());
    } else
        return false;

    // videoRender
    getRenderThread()->setVideoWidget(renderWidget);
    bRet = getRenderThread()->initializeVideoRenderer(&m_avSync, getDemuxThread()->videoTimebase());
    if (!bRet) {
        qDebug() << "initializeVideoRenderer failed.";
        return false;
    }

    // audioRender
    bRet = getAudioRenderThread()->initializeAudioRenderer(&m_avSync,
                                                           getDemuxThread()->audioTimebase(),
                                                           getDemuxThread()->audioCodecParameters());
    if (!bRet) {
        qWarning() << "initializeAudioRenderer failed.";
        return false;
    }

    return true;
}

bool ThreadManager::startAllThreads()
{
    if (!m_initialized && !initializeThreads()) {
        return false;
    }

    if (m_isPlaying) {
        return true;
    }

    // 按照正确的顺序启动所有线程
    // 1. 先启动解复用线程
    m_threads[DEMUX]->startProcess();

    // 2. 启动解码线程
    m_threads[VIDEO_DECODE]->startProcess();
    m_threads[AUDIO_DECODE]->startProcess();

    // 4. 启动渲染线程
    m_threads[VIDEO_RENDER]->startProcess();
    m_threads[AUDIO_RENDER]->startProcess();

#ifdef ENABLE_LIVE_DANMU
    // 5. 启动弹幕线程（如果需要）
    m_threads[DANMAKU]->startProcess();

    // 6. 启动直播流线程（如果是直播模式）
    m_threads[LIVE_STREAM]->startProcess();
#endif

    m_isPlaying = true;
    emit playStateChanged(m_isPlaying);

    return true;
}

void ThreadManager::pauseAllThreads()
{
    if (!m_isPlaying) {
        return;
    }

    // 暂停所有线程
    for (auto it = m_threads.begin(); it != m_threads.end(); ++it) {
        it.value()->pauseProcess();
    }

    m_isPlaying = false;
    emit playStateChanged(m_isPlaying);
}

void ThreadManager::resumeAllThreads()
{
    if (m_isPlaying) {
        return;
    }

    // 恢复所有线程
    for (auto it = m_threads.begin(); it != m_threads.end(); ++it) {
        it.value()->resumeProcess();
    }

    m_isPlaying = true;
    emit playStateChanged(m_isPlaying);
}

void ThreadManager::stopAllThreads()
{
    // 按照相反的顺序停止线程
    // 1. 先停止渲染和弹幕线程
    if (m_threads.contains(VIDEO_RENDER))
        m_threads[VIDEO_RENDER]->stopProcess();
    if (m_threads.contains(AUDIO_RENDER))
        m_threads[AUDIO_RENDER]->stopProcess();
#ifdef ENABLE_LIVE_DANMU
    if (m_threads.contains(DANMAKU))
        m_threads[DANMAKU]->stopProcess();
    if (m_threads.contains(LIVE_STREAM))
        m_threads[LIVE_STREAM]->stopProcess();
#endif

    // 2. 停止解码线程
    if (m_threads.contains(VIDEO_DECODE))
        m_threads[VIDEO_DECODE]->stopProcess();
    if (m_threads.contains(AUDIO_DECODE))
        m_threads[AUDIO_DECODE]->stopProcess();

    // 3. 最后停止解复用线程
    if (m_threads.contains(DEMUX))
        m_threads[DEMUX]->stopProcess();

    // 等待所有线程结束
    for (auto it = m_threads.begin(); it != m_threads.end(); ++it) {
        if (it.value()->isRunning()) {
            it.value()->wait();
        }
    }

    if (m_isPlaying) {
        m_isPlaying = false;
        emit playStateChanged(m_isPlaying);
    }
}

ThreadBase *ThreadManager::getThread(ThreadType type)
{
    if (m_threads.contains(type)) {
        return m_threads[type].get();
    }
    return nullptr;
}

DemuxThread *ThreadManager::getDemuxThread()
{
    return static_cast<DemuxThread *>(getThread(DEMUX));
}

VideoDecodeThread *ThreadManager::getVideoDecodeThread()
{
    return static_cast<VideoDecodeThread *>(getThread(VIDEO_DECODE));
}

AudioDecodeThread *ThreadManager::getAudioDecodeThread()
{
    return static_cast<AudioDecodeThread *>(getThread(AUDIO_DECODE));
}

RenderThread *ThreadManager::getRenderThread()
{
    return static_cast<RenderThread *>(getThread(VIDEO_RENDER));
}

AudioRenderThread *ThreadManager::getAudioRenderThread()
{
    return static_cast<AudioRenderThread *>(getThread(AUDIO_RENDER));
}

#ifdef ENABLE_LIVE_DANMU
DanmakuThread *ThreadManager::getDanmakuThread()
{
    return static_cast<DanmakuThread *>(getThread(DANMAKU));
}

LiveStreamThread *ThreadManager::getLiveStreamThread()
{
    return static_cast<LiveStreamThread *>(getThread(LIVE_STREAM));
}
#endif

void ThreadManager::setPlaybackSpeed(double speed)
{
    qInfo() << "播放速度已设置为:" << speed;
}

double ThreadManager::getPlaybackSpeed() const
{
    return 0;
}

double ThreadManager::getCurrentPlayProgress()
{
    int64_t duration = getDemuxThread()->getDuration();
    double  progress = m_avSync.getClock() * 1000 / duration;
    return progress;
}
