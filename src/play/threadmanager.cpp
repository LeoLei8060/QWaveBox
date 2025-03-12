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
    , m_playState(PlayState::StoppedState)
{}

ThreadManager::~ThreadManager()
{
    if (isPlaying())
        stopPlay();
}

bool ThreadManager::openMedia(const QString &path)
{
    auto demuxThd = getDemuxThread();
    auto bRet = demuxThd->openMedia(path);
    if (!bRet) {
        qWarning() << "openMedia failed.";
        return false;
    }
    bRet = resetThreadLinkage();
    return bRet;
}

void ThreadManager::stopPlay()
{
    auto demuxThd = getDemuxThread();
    auto videoThd = getVideoDecodeThread();
    auto audioThd = getAudioDecodeThread();
    auto vRenderThd = getRenderThread();
    auto aRenderThd = getAudioRenderThread();
    if (!demuxThd || !videoThd || !vRenderThd || !audioThd || !aRenderThd)
        return;
    // 停止播放的具体流程：
    // 停止所有线程
    stopAllThreads();
    // 渲染线程关闭渲染器
    aRenderThd->closeRenderer();
    vRenderThd->closeRenderer();
    // 解码线程关闭解码器
    audioThd->closeDecoder();
    videoThd->closeDecoder();
    // 解复用线程关闭媒体
    demuxThd->closeMedia();
}

void ThreadManager::seekToPosition(int64_t position)
{
    auto demuxThd = getDemuxThread();
    auto videoThd = getVideoDecodeThread();
    auto audioThd = getAudioDecodeThread();
    auto vRenderThd = getRenderThread();
    auto aRenderThd = getAudioRenderThread();

    if (demuxThd && isPlaying()) {
        // 1. 暂停所有线程
        videoThd->pauseProcess();
        audioThd->pauseProcess();
        demuxThd->pauseProcess();
        vRenderThd->pauseProcess();
        aRenderThd->pauseProcess();

        // 2. 执行 Seek 操作
        demuxThd->seekTo(position);

        // 3. 刷新解码器（需确保线程已暂停）
        if (videoThd->isPaused() && audioThd->isPaused()) {
            m_avSync.initClock();
            audioThd->flush();
            videoThd->flush();
        }

        // 4. 恢复线程
        demuxThd->resumeProcess();
        videoThd->resumeProcess();
        audioThd->resumeProcess();
        vRenderThd->resumeProcess();
        aRenderThd->resumeProcess();
    }
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
                emit sigPlayError(error);
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

        // 关联线程间数据
        auto demuxThd = getDemuxThread();
        auto videoThd = getVideoDecodeThread();
        auto audioThd = getAudioDecodeThread();
        auto vRenderThd = getRenderThread();
        auto aRenderThd = getAudioRenderThread();
        if (!demuxThd || !videoThd || !vRenderThd || !audioThd || !aRenderThd)
            return false;

        // demux -> decode (packetQueue)
        videoThd->setPacketQueue(demuxThd->videoPacketQueue());
        audioThd->setPacketQueue(demuxThd->audioPacketQueue());
        // video decode -> video render (frameQueue)
        vRenderThd->setVideoFrameQueue(videoThd->getFrameQueue());
        // audio decode -> audio render (frameQueue)
        aRenderThd->setAudioFrameQueue(audioThd->getFrameQueue());
        // sync
        vRenderThd->setSync(&m_avSync);
        aRenderThd->setSync(&m_avSync);

        m_initialized = true;
        return true;
    } catch (const std::exception &e) {
        qWarning() << "初始化线程时发生异常：" << e.what();
        return false;
    }
}

bool ThreadManager::startAllThreads()
{
    if (!m_initialized && !initializeThreads()) {
        return false;
    }

    if (isPlaying()) {
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

    m_playState = PlayState::PlayingState;
    emit sigPlayStateChanged(m_playState);
    qDebug() << "startAllThreads ...";
    return true;
}

void ThreadManager::pauseAllThreads()
{
    if (!isPlaying()) {
        return;
    }

    // 暂停所有线程
    for (auto it = m_threads.begin(); it != m_threads.end(); ++it) {
        it.value()->pauseProcess();
    }

    m_playState = PlayState::PausedState;
    emit sigPlayStateChanged(m_playState);
}

void ThreadManager::resumeAllThreads()
{
    if (isPlaying()) {
        return;
    }

    // 恢复所有线程
    for (auto it = m_threads.begin(); it != m_threads.end(); ++it) {
        it.value()->resumeProcess();
    }

    m_playState = PlayState::PlayingState;
    emit sigPlayStateChanged(m_playState);
}

void ThreadManager::stopAllThreads()
{
    qDebug() << __FUNCTION__;
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

    if (isPlaying()) {
        m_playState = PlayState::StoppedState;
        emit sigPlayStateChanged(m_playState);
    }
}

ThreadBase *ThreadManager::getThread(ThreadType type)
{
    if (m_threads.contains(type)) {
        return m_threads[type].get();
    }
    return nullptr;
}

void ThreadManager::setVideoRenderObj(SDLWidget *obj)
{
    auto vRender = getRenderThread();
    if (vRender)
        vRender->setVideoWidget(obj);
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
    double progress = m_avSync.getClock() * 1000;
    return progress;
}

int64_t ThreadManager::getPlayDuration()
{
    return (int64_t) (m_avSync.getClock() * 1000);
}

void ThreadManager::setVolume(int volume)
{
    auto audioThd = getAudioRenderThread();
    if (audioThd)
        audioThd->setVolume(volume);
    if (0 == volume && m_voiceState != VoiceState::MuteState) {
        m_voiceState = VoiceState::MuteState;
        emit sigVoiceStateChanged(m_voiceState);
    } else if (0 != volume && m_voiceState == VoiceState::MuteState) {
        m_voiceState = VoiceState::NormalState;
        emit sigVoiceStateChanged(m_voiceState);
    }
}

bool ThreadManager::resetThreadLinkage()
{
    bool bRet = false;
    // reset sync
    m_avSync.initClock();

    // demux -> decode (packetQueue)
    auto demuxThd = getDemuxThread();
    auto videoThd = getVideoDecodeThread();
    auto audioThd = getAudioDecodeThread();
    auto vRenderThd = getRenderThread();
    auto aRenderThd = getAudioRenderThread();
    if (!demuxThd || !videoThd || !vRenderThd || !audioThd || !aRenderThd)
        return false;
    videoThd->openDecoder(demuxThd->getVideoStreamIndex(), demuxThd->videoCodecParameters());
    audioThd->openDecoder(demuxThd->getAudioStreamIndex(), demuxThd->audioCodecParameters());

    // videoRender
    bRet = vRenderThd->initializeVideoRenderer(getDemuxThread()->videoTimebase());
    if (!bRet) {
        qDebug() << "initializeVideoRenderer failed.";
        return false;
    }

    // audioRender
    bRet = aRenderThd->initializeAudioRenderer(getDemuxThread()->audioTimebase(),
                                               getDemuxThread()->audioCodecParameters());
    if (!bRet) {
        qWarning() << "initializeAudioRenderer failed.";
        return false;
    }

    return true;
}
