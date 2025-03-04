#ifndef SYNCTHREAD_H
#define SYNCTHREAD_H

#include "syncdata.h"
#include "threadbase.h"

#include <atomic>
#include <QMutex>

/**
 * @brief 同步线程类 - 负责音视频同步
 */
class SyncThread : public ThreadBase
{
    Q_OBJECT
public:
    // 同步模式
    enum SyncMode {
        SYNC_AUDIO,   // 以音频为基准同步视频
        SYNC_VIDEO,   // 以视频为基准同步音频
        SYNC_EXTERNAL // 以外部时钟为基准同步
    };

    explicit SyncThread(QObject *parent = nullptr);
    ~SyncThread() override;

    // 初始化线程
    bool initialize() override;

    // 设置同步数据
    void setSyncData(std::shared_ptr<SyncData> data);

    // 设置同步模式
    void setSyncMode(SyncMode mode);

    // 获取当前同步模式
    SyncMode getSyncMode() const;

    // 设置音频时钟
    void setAudioClock(int64_t pts);

    // 设置视频时钟
    void setVideoClock(int64_t pts);

    // 设置外部时钟
    void setExternalClock(int64_t pts);

    // 获取主时钟（根据同步模式选择对应的时钟）
    int64_t getMasterClock() const;

    // 计算视频延迟
    double calculateVideoDelay(int64_t videoPts);

    // 计算音频延迟
    double calculateAudioDelay(int64_t audioPts);

    // 重置时钟
    void resetClocks();

signals:
    // 同步事件
    void syncEvent(int64_t masterClock, double videoDelay, double audioDelay);

protected:
    // 线程处理函数
    void process() override;

private:
    // 清理资源
    void cleanup();

    // 更新主时钟
    void updateMasterClock();

private:
    // 同步模式
    SyncMode m_syncMode{SYNC_VIDEO};

    // 各种时钟（毫秒）
    std::atomic<int64_t> m_audioClock{0};    // 音频时钟
    std::atomic<int64_t> m_videoClock{0};    // 视频时钟
    std::atomic<int64_t> m_externalClock{0}; // 外部时钟
    std::atomic<int64_t> m_masterClock{0};   // 主时钟

    // 延迟时间
    double m_videoDelay{0.0};
    double m_audioDelay{0.0};

    // 同步阈值（毫秒）
    double m_syncThreshold{10.0}; // 同步阈值

    // 同步锁
    QMutex m_mutex;

    std::shared_ptr<SyncData> m_syncData{nullptr};
};

#endif // SYNCTHREAD_H
