#include "syncthread.h"
#include <QDebug>

SyncThread::SyncThread(QObject *parent)
    : ThreadBase(parent)
    , m_syncMode(SYNC_AUDIO)
    , m_audioClock(0)
    , m_videoClock(0)
    , m_externalClock(0)
    , m_masterClock(0)
    , m_videoDelay(0.0)
    , m_audioDelay(0.0)
    , m_syncThreshold(10.0)
    , m_playbackSpeed(1.0)
    , m_audioSampleRate(44100)
{}

SyncThread::~SyncThread()
{
    stopProcess();
    wait();
}

bool SyncThread::initialize()
{
    resetClocks();
    return true;
}

void SyncThread::setSyncData(std::shared_ptr<SyncData> data)
{
    m_syncData = data;
}

void SyncThread::setSyncMode(SyncMode mode)
{
    QMutexLocker locker(&m_mutex);

    if (m_syncMode != mode) {
        m_syncMode = mode;

        // 输出日志
        QString modeStr;
        switch (m_syncMode) {
        case SYNC_AUDIO:
            modeStr = "音频时钟";
            break;
        case SYNC_VIDEO:
            modeStr = "视频时钟";
            break;
        case SYNC_EXTERNAL:
            modeStr = "外部时钟";
            break;
        }

        qInfo() << "同步模式已更改为:" << modeStr;

        // 更新主时钟
        updateMasterClock();
    }
}

SyncThread::SyncMode SyncThread::getSyncMode() const
{
    return m_syncMode;
}

void SyncThread::setAudioClock(int64_t pts)
{
    m_audioClock = pts;

    // 如果当前同步模式是音频，则更新主时钟
    if (m_syncMode == SYNC_AUDIO) {
        m_masterClock = pts;
    }
}

void SyncThread::setVideoClock(int64_t pts)
{
    m_videoClock = pts;

    // 如果当前同步模式是视频，则更新主时钟
    if (m_syncMode == SYNC_VIDEO) {
        m_masterClock = pts;
    }
}

void SyncThread::setExternalClock(int64_t pts)
{
    m_externalClock = pts;

    // 如果当前同步模式是外部，则更新主时钟
    if (m_syncMode == SYNC_EXTERNAL) {
        m_masterClock = pts;
    }
}

void SyncThread::setPlaybackSpeed(double speed)
{
    QMutexLocker locker(&m_mutex);

    if (speed <= 0.0) {
        qWarning() << "播放速度设置无效，必须大于0，当前值:" << speed;
        return;
    }

    if (m_playbackSpeed != speed) {
        m_playbackSpeed = speed;
        qInfo() << "播放速度已设置为:" << m_playbackSpeed;
    }
}

void SyncThread::setAudioSampleRate(int sampleRate)
{
    QMutexLocker locker(&m_mutex);

    if (sampleRate <= 0) {
        qWarning() << "音频采样率设置无效，必须大于0，当前值:" << sampleRate;
        return;
    }

    if (m_audioSampleRate != sampleRate) {
        m_audioSampleRate = sampleRate;
        qInfo() << "音频采样率已设置为:" << m_audioSampleRate;
    }
}

int64_t SyncThread::getMasterClock() const
{
    return m_masterClock;
}

double SyncThread::calculateVideoDelay(int64_t videoPts)
{
    // 如果视频是主时钟，则不需要延迟
    if (m_syncMode == SYNC_VIDEO) {
        return 0.0;
    }

    // 计算与主时钟的差异，考虑播放速度
    double  speedFactor = (m_playbackSpeed != 0.0) ? (1.0 / m_playbackSpeed) : 1.0;
    int64_t diff = videoPts - m_masterClock;

    // 计算基本帧延迟（毫秒）
    double basicFrameDelay = 1000.0 / 30.0; // 假设30fps，约33.33ms每帧
    
    // 如果差异过大，直接跳过
    if (abs(diff) > 1000) { // 如果差异大于1秒
        qWarning() << "视频与主时钟差异过大:" << diff << "ms, 将跳过同步" << videoPts
                   << m_masterClock;
        return -1.0; // 返回负值表示需要跳过此帧
    }

    // 计算延迟（毫秒），正值表示需要等待，负值表示需要加速
    // 根据播放速度调整延迟时间
    m_videoDelay = diff * speedFactor;
    
    // 添加基本帧延迟以控制播放速度
    if (m_videoDelay >= 0) {
        // 如果已经需要等待，则加上基本帧延迟
        m_videoDelay += (basicFrameDelay / m_playbackSpeed);
    } else if (m_videoDelay > -basicFrameDelay) {
        // 如果需要加速但差异不大，则使用较小的延迟
        m_videoDelay = (basicFrameDelay / m_playbackSpeed) / 2.0;
    }

    // 如果需要等待的时间小于同步阈值，则使用最小延迟
    if (fabs(m_videoDelay) < m_syncThreshold) {
        m_videoDelay = (basicFrameDelay / m_playbackSpeed) / 2.0;
    }

    return m_videoDelay;
}

double SyncThread::calculateAudioDelay(int64_t audioPts)
{
    // 如果音频是主时钟，则不需要延迟
    if (m_syncMode == SYNC_AUDIO) {
        return 0.0;
    }

    // 计算与主时钟的差异，考虑播放速度
    double  speedFactor = (m_playbackSpeed != 0.0) ? (1.0 / m_playbackSpeed) : 1.0;
    int64_t diff = audioPts - m_masterClock;

    // 计算基本帧延迟（毫秒）
    // 使用音频采样率计算更精确的延迟
    double audioFrameDelay = 1000.0 * 1024.0 / m_audioSampleRate; // 基于音频帧大小和采样率
    
    // 如果差异过大，直接跳过
    if (abs(diff) > 1000) { // 如果差异大于1秒
        qWarning() << "音频与主时钟差异过大:" << diff << "ms, 将跳过同步" << audioPts
                   << m_masterClock;
        return -1.0; // 返回负值表示需要跳过此帧
    }

    // 计算延迟（毫秒），正值表示需要等待，负值表示需要加速
    // 根据播放速度调整延迟时间
    m_audioDelay = diff * speedFactor;
    
    // 添加基本帧延迟以控制播放速度
    if (m_audioDelay >= 0) {
        // 如果已经需要等待，则加上基本帧延迟
        m_audioDelay += (audioFrameDelay / m_playbackSpeed);
    } else if (m_audioDelay > -audioFrameDelay) {
        // 如果需要加速但差异不大，则使用较小的延迟
        m_audioDelay = (audioFrameDelay / m_playbackSpeed) / 2.0;
    }

    // 如果需要等待的时间小于同步阈值，则使用最小延迟
    if (fabs(m_audioDelay) < m_syncThreshold) {
        m_audioDelay = (audioFrameDelay / m_playbackSpeed) / 2.0;
    }

    return m_audioDelay;
}

void SyncThread::resetClocks()
{
    QMutexLocker locker(&m_mutex);

    m_audioClock = 0;
    m_videoClock = 0;
    m_externalClock = 0;
    m_masterClock = 0;
    m_videoDelay = 0.0;
    m_audioDelay = 0.0;

    qInfo() << "所有时钟已重置";
}

void SyncThread::process()
{
    if (m_syncData) {
        // 获取当前时间戳
        int64_t videoPts = m_syncData->getVideoPts();
        int64_t audioPts = m_syncData->getAudioPts();

        // 更新时钟
        setVideoClock(videoPts);
        setAudioClock(audioPts);

        // 获取主时钟
        int64_t masterClock = getMasterClock();

        // 计算延迟，考虑播放速度
        double videoDelay = calculateVideoDelay(videoPts);
        double audioDelay = calculateAudioDelay(audioPts);

        // 更新 SyncData
        m_syncData->setSyncInfo(masterClock, videoDelay, audioDelay);

        // 发出同步事件
        emit syncEvent(masterClock, videoDelay, audioDelay);
    }

    // 同步处理不需要太频繁，每10ms执行一次即可
    msleep(10);
}

void SyncThread::cleanup()
{
    m_syncData = nullptr;
}

void SyncThread::updateMasterClock()
{
    switch (m_syncMode) {
    case SYNC_AUDIO:
        m_masterClock.store(m_audioClock.load());
        break;
    case SYNC_VIDEO:
        m_masterClock.store(m_videoClock.load());
        break;
    case SYNC_EXTERNAL:
        m_masterClock.store(m_externalClock.load());
        break;
    }
}
