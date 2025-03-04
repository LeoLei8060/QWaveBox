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

    // 计算与主时钟的差异
    int64_t diff = videoPts - m_masterClock;

    // 如果差异过大，直接跳过
    if (abs(diff) > 1000) { // 如果差异大于1秒
        qWarning() << "视频与主时钟差异过大:" << diff << "ms, 将跳过同步" << videoPts
                   << m_masterClock;
        return -1.0; // 返回负值表示需要跳过此帧
    }

    // 计算延迟（毫秒），正值表示需要等待，负值表示需要加速
    m_videoDelay = diff;

    // 如果需要等待的时间小于同步阈值，则不进行调整
    if (fabs(m_videoDelay) < m_syncThreshold) {
        m_videoDelay = 0.0;
    }

    return m_videoDelay;
}

double SyncThread::calculateAudioDelay(int64_t audioPts)
{
    // 如果音频是主时钟，则不需要延迟
    if (m_syncMode == SYNC_AUDIO) {
        return 0.0;
    }

    // 计算与主时钟的差异
    int64_t diff = audioPts - m_masterClock;

    // 如果差异过大，直接跳过
    if (abs(diff) > 1000) { // 如果差异大于1秒
        qWarning() << "音频与主时钟差异过大:" << diff << "ms, 将跳过同步";
        return -1.0; // 返回负值表示需要跳过此帧
    }

    // 计算延迟（毫秒），正值表示需要等待，负值表示需要加速
    m_audioDelay = diff;

    // 如果需要等待的时间小于同步阈值，则不进行调整
    if (fabs(m_audioDelay) < m_syncThreshold) {
        m_audioDelay = 0.0;
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

        // 计算延迟
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
    resetClocks();
}

void SyncThread::updateMasterClock()
{
    QMutexLocker locker(&m_mutex);

    // 根据同步模式选择主时钟
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
