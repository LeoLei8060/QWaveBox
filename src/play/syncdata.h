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
        , m_playbackSpeed(1.0)
        , m_audioSampleRate(44100)
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
    
    // 获取主时钟
    int64_t getMasterClock() const
    {
        QMutexLocker locker(&m_mutex);
        return m_masterClock;
    }
    
    // 设置主时钟
    void setMasterClock(int64_t masterClock)
    {
        QMutexLocker locker(&m_mutex);
        m_masterClock = masterClock;
    }

    // 获取视频延迟
    double getVideoDelay() const
    {
        QMutexLocker locker(&m_mutex);
        return m_videoDelay;
    }

    // 获取音频延迟
    double getAudioDelay() const
    {
        QMutexLocker locker(&m_mutex);
        return m_audioDelay;
    }
    
    // 设置播放速度
    void setPlaybackSpeed(double speed)
    {
        QMutexLocker locker(&m_mutex);
        if (speed <= 0.0) {
            return; // 忽略无效速度
        }
        m_playbackSpeed = speed;
    }
    
    // 获取播放速度
    double getPlaybackSpeed() const
    {
        QMutexLocker locker(&m_mutex);
        return m_playbackSpeed;
    }
    
    // 设置音频采样率
    void setAudioSampleRate(int sampleRate)
    {
        QMutexLocker locker(&m_mutex);
        if (sampleRate <= 0) {
            return; // 忽略无效采样率
        }
        m_audioSampleRate = sampleRate;
    }
    
    // 获取音频采样率
    int getAudioSampleRate() const
    {
        QMutexLocker locker(&m_mutex);
        return m_audioSampleRate;
    }

private:
    mutable QMutex m_mutex;
    int64_t        m_videoPts;
    int64_t        m_audioPts;
    int64_t        m_masterClock;
    double         m_videoDelay;
    double         m_audioDelay;
    double         m_playbackSpeed;  // 播放速度
    int            m_audioSampleRate; // 音频采样率
};

#endif // SYNCDATA_H
