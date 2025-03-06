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
    m_audioClock.store(pts);

    // 如果当前同步模式是音频，则更新主时钟
    if (m_syncMode == SYNC_AUDIO) {
        m_masterClock.store(pts * 0.02267);
    }
}

void SyncThread::setVideoClock(int64_t pts)
{
    m_videoClock.store(pts);

    // 如果当前同步模式是视频，则更新主时钟
    if (m_syncMode == SYNC_VIDEO) {
        m_masterClock.store(pts);
    }
}

void SyncThread::setExternalClock(int64_t pts)
{
    m_externalClock.store(pts);

    // 如果当前同步模式是外部，则更新主时钟
    if (m_syncMode == SYNC_EXTERNAL) {
        m_masterClock.store(pts);
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
    return m_masterClock.load(); // 使用load()方法读取原子变量
}

double SyncThread::calculateVideoDelay(int64_t videoPts)
{
    // 如果视频是主时钟，则不需要延迟
    if (m_syncMode == SYNC_VIDEO) {
        return 0.0;
    }

    // 计算与主时钟的差异，考虑播放速度
    double  speedFactor = (m_playbackSpeed != 0.0) ? (1.0 / m_playbackSpeed) : 1.0;
    int64_t diff = videoPts - m_masterClock.load(); // 使用load()方法读取原子变量

    // 计算基本帧延迟（毫秒）
    double basicFrameDelay = 1000.0 / 30.0; // 假设30fps，约33.33ms每帧

    // 如果差异过大，直接跳过
    if (abs(diff) > 1000) { // 如果差异大于1秒
        // qWarning() << "视频与主时钟差异过大:" << diff << "ms, 将跳过同步" << videoPts
        //            << m_masterClock.load(); // 使用load()方法读取原子变量
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

std::pair<double, double> SyncThread::calculateDelay(
    int64_t videoPts, int videoNum, int videoDen, int64_t audioPts, int audioNum, int audioDen)
{
    // NOTE: 返回值为[double, double]，前者是视频延时，后者是音频延时
    std::pair<double, double> result = {0.0, 0.0};
    double                    videoBase = videoNum * 1.0 / videoDen;
    double                    audioBase = audioNum * 1.0 / audioDen;
    double                    videoTime_ms = videoPts /* * videoBase * 1000*/;
    double                    audioTime_ms = audioPts * 0.02267 /*audioBase * 1000*/;
    if (m_syncMode == SYNC_AUDIO) {
        // m_masterClock.store(audioTime_ms);
        qDebug() << "master: " << audioTime_ms << audioPts << videoPts;
        m_syncData->setMasterClock(audioTime_ms);
        // 计算视频延时
        // double diff = videoTime_ms - audioTime_ms;
        // if (videoPts > 0)
        //     qDebug() << diff << videoPts << videoTime_ms << audioPts << audioTime_ms;
        // if (diff > 1000)
        //     result.first = 10;
        // else if (diff > 200)
        //     result.first = 1;
    } else if (m_syncMode == SYNC_VIDEO) {
        // 音频不用调整
    }
    return result;
}

double SyncThread::calculateAudioDelay(int64_t audioPts)
{
    // 如果音频是主时钟，则不需要延迟
    if (m_syncMode == SYNC_AUDIO) {
        return 0.0;
    }

    // 计算与主时钟的差异，考虑播放速度
    double  speedFactor = (m_playbackSpeed != 0.0) ? (1.0 / m_playbackSpeed) : 1.0;
    int64_t diff = audioPts - m_masterClock.load(); // 使用load()方法读取原子变量

    // 计算基本帧延迟（毫秒）
    // 使用音频采样率计算更精确的延迟
    double audioFrameDelay = 1000.0 * 1024.0 / m_audioSampleRate; // 基于音频帧大小和采样率

    // 如果差异过大，直接跳过
    if (abs(diff) > 1000) { // 如果差异大于1秒
        qWarning() << "音频与主时钟差异过大:" << diff << "ms, 将跳过同步" << audioPts
                   << m_masterClock.load(); // 使用load()方法读取原子变量
        return -1.0;                        // 返回负值表示需要跳过此帧
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

// double SyncThread::calculateAudioDelay(int64_t audioPts, int num, int den)
// {

// }

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
    // msleep(1);
    // return;
    if (m_syncData) {
        // 获取当前时间戳
        auto [videoPts, videoBase_num, videoBase_den] = m_syncData->getVideoData();
        auto [audioPts, audioBase_num, audioBase_den] = m_syncData->getAudioData();

        // 更新本地时钟
        setVideoClock(videoPts);
        setAudioClock(audioPts);
        int64_t tm = audioPts * 0.02267;
        m_syncData->setMasterClock(tm);
        // qDebug() << "sync: " << tm << audioPts;

        // 获取主时钟值
        // int64_t masterClock = getMasterClock();

        // 获取播放速度和音频采样率
        // double playbackSpeed = m_syncData->getPlaybackSpeed();
        // int    audioSampleRate = m_syncData->getAudioSampleRate();

        // // 更新内部状态
        // if (playbackSpeed > 0.0) {
        //     m_playbackSpeed = playbackSpeed;
        // }
        // if (audioSampleRate > 0) {
        //     m_audioSampleRate = audioSampleRate;
        // }

        // 计算延迟，考虑播放速度
        // auto [videoDelay, audioDelay] = calculateDelay(videoPts,
        //                                                videoBase_num,
        //                                                videoBase_den,
        //                                                audioPts,
        //                                                audioBase_num,
        //                                                audioBase_den);

        // 更新 SyncData
        // m_syncData->setSyncInfo(masterClock, videoDelay, audioDelay);

        // 发出同步事件
        // emit syncEvent(masterClock, videoDelay, audioDelay);
    }

    // 同步处理不需要太频繁，每10ms执行一次即可
    msleep(1);
}

void SyncThread::cleanup()
{
    m_syncData = nullptr;
}

void SyncThread::updateMasterClock()
{
    QMutexLocker locker(&m_mutex);

    // 根据同步模式选择主时钟
    switch (m_syncMode) {
    case SYNC_AUDIO:
        m_masterClock.store(m_audioClock.load()); // 使用load()方法读取原子变量
        break;
    case SYNC_VIDEO:
        m_masterClock.store(m_videoClock.load()); // 使用load()方法读取原子变量
        break;
    case SYNC_EXTERNAL:
        m_masterClock.store(m_externalClock.load()); // 使用load()方法读取原子变量
        break;
    }
}
