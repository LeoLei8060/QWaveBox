#ifndef SYNCDATA_H
#define SYNCDATA_H

#include <QMutex>
#include <QReadWriteLock>

/**
 * @brief 同步数据类 - 用于渲染线程和同步线程之间共享同步数据
 */
class SyncData
{
    struct AVData
    {
        int64_t pts_ = 0;
        int     num_ = 0;
        int     den_ = 0;
    };

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
    void setVideoData(int64_t pts, int num, int den)
    {
        QWriteLocker locker(&m_mutex);
        m_videoData.pts_ = pts;
        m_videoData.num_ = num;
        m_videoData.den_ = den;
    }

    // 设置音频时间戳
    void setAudioData(int64_t pts, int num, int den)
    {
        QWriteLocker locker(&m_mutex);
        m_audioData.pts_ = pts;
        m_audioData.num_ = num;
        m_audioData.den_ = den;
    }

    AVData getAudioData() const
    {
        QReadLocker locker(&m_mutex);
        return m_audioData;
    }

    AVData getVideoData() const
    {
        QReadLocker locker(&m_mutex);
        return m_videoData;
    }

    // 设置主时钟和延迟值
    void setSyncInfo(int64_t masterClock, double videoDelay, double audioDelay)
    {
        QWriteLocker locker(&m_mutex);
        m_masterClock = masterClock;
        m_videoDelay = videoDelay;
        m_audioDelay = audioDelay;
    }

    // 获取同步信息
    void getSyncInfo(int64_t &masterClock, double &videoDelay, double &audioDelay) const
    {
        QReadLocker locker(&m_mutex);
        masterClock = m_masterClock;
        videoDelay = m_videoDelay;
        audioDelay = m_audioDelay;
    }

    // 获取主时钟
    int64_t getMasterClock() const
    {
        QReadLocker locker(&m_mutex);
        return m_masterClock;
    }

    // 设置主时钟
    void setMasterClock(int64_t masterClock)
    {
        QWriteLocker locker(&m_mutex);
        m_masterClock = masterClock;
    }

    // 获取视频延迟
    double getVideoDelay() const
    {
        QReadLocker locker(&m_mutex);
        return m_videoDelay;
    }

    // 获取音频延迟
    double getAudioDelay() const
    {
        QReadLocker locker(&m_mutex);
        return m_audioDelay;
    }

    // 设置播放速度
    void setPlaybackSpeed(double speed)
    {
        QWriteLocker locker(&m_mutex);
        if (speed <= 0.0) {
            return; // 忽略无效速度
        }
        m_playbackSpeed = speed;
    }

    // 获取播放速度
    double getPlaybackSpeed() const
    {
        QReadLocker locker(&m_mutex);
        return m_playbackSpeed;
    }

    // 设置音频采样率
    void setAudioSampleRate(int sampleRate)
    {
        QWriteLocker locker(&m_mutex);
        if (sampleRate <= 0) {
            return; // 忽略无效采样率
        }
        m_audioSampleRate = sampleRate;
    }

    // 获取音频采样率
    int getAudioSampleRate() const
    {
        QReadLocker locker(&m_mutex);
        return m_audioSampleRate;
    }

private:
    mutable QReadWriteLock m_mutex;
    int64_t                m_videoPts;
    int64_t                m_audioPts;
    int64_t                m_masterClock;
    double                 m_videoDelay;
    double                 m_audioDelay;
    int                    m_videoTmBase_num;
    int                    m_videoTmBase_den;
    int                    m_audioTmBase_num;
    int                    m_audioTmBase_den;
    double                 m_playbackSpeed;   // 播放速度
    int                    m_audioSampleRate; // 音频采样率

    AVData m_videoData;
    AVData m_audioData;
};

#endif // SYNCDATA_H
