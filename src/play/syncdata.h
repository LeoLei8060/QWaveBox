#ifndef SYNCDATA_H
#define SYNCDATA_H

#include <QMutex>

/**
 * @brief 同步数据类 - 用于渲染线程和同步线程之间共享同步数据
 */
class SyncData
{
public:
    SyncData()
        : m_videoPts(0)
        , m_audioPts(0)
        , m_masterClock(0)
        , m_videoDelay(0.0)
        , m_audioDelay(0.0)
    {}

    // 设置视频时间戳
    void setVideoPts(int64_t pts)
    {
        QMutexLocker locker(&m_mutex);
        m_videoPts = pts;
    }

    // 设置音频时间戳
    void setAudioPts(int64_t pts)
    {
        QMutexLocker locker(&m_mutex);
        m_audioPts = pts;
    }

    // 获取视频时间戳
    int64_t getVideoPts() const
    {
        QMutexLocker locker(&m_mutex);
        return m_videoPts;
    }

    // 获取音频时间戳
    int64_t getAudioPts() const
    {
        QMutexLocker locker(&m_mutex);
        return m_audioPts;
    }

    // 设置主时钟和延迟值
    void setSyncInfo(int64_t masterClock, double videoDelay, double audioDelay)
    {
        QMutexLocker locker(&m_mutex);
        m_masterClock = masterClock;
        m_videoDelay = videoDelay;
        m_audioDelay = audioDelay;
    }

    // 获取同步信息
    void getSyncInfo(int64_t &masterClock, double &videoDelay, double &audioDelay) const
    {
        QMutexLocker locker(&m_mutex);
        masterClock = m_masterClock;
        videoDelay = m_videoDelay;
        audioDelay = m_audioDelay;
    }

private:
    mutable QMutex m_mutex;
    int64_t        m_videoPts;
    int64_t        m_audioPts;
    int64_t        m_masterClock;
    double         m_videoDelay;
    double         m_audioDelay;
};

#endif // SYNCDATA_H
