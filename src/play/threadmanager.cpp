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
    , m_syncData(nullptr)
{}

ThreadManager::~ThreadManager()
{
    stopAllThreads();
}

bool ThreadManager::openMedia(const QString &path)
{
    auto demuxThd = getDemuxThread();
    auto bRet = demuxThd->openMedia(path);
    if (!bRet)
        return false;

    auto videoThd = getVideoDecodeThread();
    auto audioThd = getAudioDecodeThread();
    bool bRet1 = videoThd->openDecoder(demuxThd->getVideoStreamIndex(),
                                       demuxThd->videoCodecParameters());
    bool bRet2 = audioThd->openDecoder(demuxThd->getAudioStreamIndex(),
                                       demuxThd->audioCodecParameters());

    return bRet1 && bRet2;
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
        m_threads[SYNC] = std::make_shared<SyncThread>();
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

        // 设置线程之间的关联
        // 例如: 将解复用线程的输出连接到解码线程的输入
        // 具体的连接逻辑需要根据实际数据流向进行设置
        auto demuxThd = getDemuxThread();
        auto videoThd = getVideoDecodeThread();
        auto audioThd = getAudioDecodeThread();
        if (demuxThd && videoThd && audioThd) {
            videoThd->setPacketQueue(demuxThd->videoPacketQueue());
            audioThd->setPacketQueue(demuxThd->audioPacketQueue());
        }

        auto vRenderThd = getRenderThread();
        auto aRenderThd = getAudioRenderThread();
        if (videoThd && vRenderThd) {
            vRenderThd->setVideoFrameQueue(videoThd->getFrameQueue());
        }
        if (audioThd && aRenderThd) {
            aRenderThd->setAudioFrameQueue(audioThd->getFrameQueue());
        }

        m_syncData = std::make_shared<SyncData>();

        // 设置默认的播放速度和音频采样率
        m_syncData->setPlaybackSpeed(1.0);

        // 从音频解码线程获取实际采样率（如果可用）
        int sampleRate = 44100; // 默认采样率
        if (audioThd) {
            int decoderSampleRate = audioThd->getSampleRate();
            if (decoderSampleRate > 0) {
                sampleRate = decoderSampleRate;
                qInfo() << "从解码器获取音频采样率:" << sampleRate << "Hz";
            } else {
                qInfo() << "使用默认音频采样率:" << sampleRate << "Hz";
            }
        }
        m_syncData->setAudioSampleRate(sampleRate);

        auto syncThd = getSyncThread();
        if (vRenderThd && syncThd && aRenderThd) {
            vRenderThd->setSyncData(m_syncData);
            aRenderThd->setSyncData(m_syncData);
            syncThd->setSyncData(m_syncData);

            // 确保各线程使用相同的播放速度和采样率
            // renderThd->setPlaybackSpeed(1.0);
            syncThd->setPlaybackSpeed(1.0);
            syncThd->setAudioSampleRate(sampleRate);

            // connect(syncThd, &SyncThread::syncEvent, vRenderThd, &RenderThread::onSyncEvent);
        }

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

    if (m_isPlaying) {
        return true;
    }

    // 按照正确的顺序启动所有线程
    // 1. 先启动解复用线程
    m_threads[DEMUX]->startProcess();

    // 2. 启动解码线程
    m_threads[VIDEO_DECODE]->startProcess();
    m_threads[AUDIO_DECODE]->startProcess();

    // 3. 启动同步线程
    m_threads[SYNC]->startProcess();

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

    // 2. 停止同步线程
    if (m_threads.contains(SYNC))
        m_threads[SYNC]->stopProcess();

    // 3. 停止解码线程
    if (m_threads.contains(VIDEO_DECODE))
        m_threads[VIDEO_DECODE]->stopProcess();
    if (m_threads.contains(AUDIO_DECODE))
        m_threads[AUDIO_DECODE]->stopProcess();

    // 4. 最后停止解复用线程
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

SyncThread *ThreadManager::getSyncThread()
{
    return static_cast<SyncThread *>(getThread(SYNC));
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
    if (!m_syncData) {
        qWarning() << "同步数据对象尚未初始化，无法设置播放速度";
        return;
    }

    if (speed <= 0.0) {
        qWarning() << "播放速度设置无效，必须大于0，当前值:" << speed;
        return;
    }

    // 设置同步数据中的播放速度
    m_syncData->setPlaybackSpeed(speed);

    // 更新同步线程的播放速度
    auto syncThd = getSyncThread();
    if (syncThd) {
        syncThd->setPlaybackSpeed(speed);
    }

    // 更新渲染线程的播放速度
    // auto renderThd = getRenderThread();
    // if (renderThd) {
    //     renderThd->setPlaybackSpeed(speed);
    // }

    qInfo() << "播放速度已设置为:" << speed;
}

double ThreadManager::getPlaybackSpeed() const
{
    if (!m_syncData) {
        qWarning() << "同步数据对象尚未初始化，无法获取播放速度";
        return 1.0; // 默认播放速度
    }

    return m_syncData->getPlaybackSpeed();
}

void ThreadManager::setAudioSampleRate(int sampleRate)
{
    if (!m_syncData) {
        qWarning() << "同步数据对象尚未初始化，无法设置音频采样率";
        return;
    }

    if (sampleRate <= 0) {
        qWarning() << "音频采样率设置无效，必须大于0，当前值:" << sampleRate;
        return;
    }

    // 设置同步数据中的音频采样率
    m_syncData->setAudioSampleRate(sampleRate);

    // 更新同步线程的音频采样率
    auto syncThd = getSyncThread();
    if (syncThd) {
        syncThd->setAudioSampleRate(sampleRate);
    }

    qInfo() << "音频采样率已设置为:" << sampleRate << "Hz";
}

int ThreadManager::getAudioSampleRate() const
{
    if (!m_syncData) {
        qWarning() << "同步数据对象尚未初始化，无法获取音频采样率";
        return 44100; // 默认采样率
    }

    return m_syncData->getAudioSampleRate();
}
