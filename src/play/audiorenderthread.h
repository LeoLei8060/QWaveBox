#ifndef AUDIORENDERTHREAD_H
#define AUDIORENDERTHREAD_H

#include "syncdata.h"
#include "threadbase.h"

#include <memory>
#include <QMutex>
#include <QQueue>

extern "C" {
#include <SDL.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

class AVFrameQueue;

/**
 * @brief 音频渲染线程类 - 负责将解码后音频播放
 */
class AudioRenderThread : public ThreadBase
{
    Q_OBJECT
public:
    explicit AudioRenderThread(QObject *parent = nullptr);
    ~AudioRenderThread() override;

    // 初始化线程
    bool initialize() override;

    void setSyncData(std::shared_ptr<SyncData> data);

    // 设置音频帧队列
    void setAudioFrameQueue(AVFrameQueue *queue);

    // 初始化音频渲染器
    bool initializeAudioRenderer(int sampleRate, int channels, int64_t channelLayout);

    // 关闭渲染器
    void closeRenderer();

    // 获取当前音频时间戳
    int64_t getCurrentAudioPts() const;

protected:
    // 线程处理函数
    void process() override;

private:
    // 渲染音频帧
    bool renderAudioFrame(AVFrame *frame);

    // 清理资源
    void cleanup();

    // 回调
    void        audioCallback(Uint8 *stream, int len);
    static void sdlAudioCallback(void *userdata, Uint8 *stream, int len);

private:
    // 帧队列
    AVFrameQueue *m_audioFrameQueue{nullptr};

    // 是否已初始化
    bool m_audioInitialized{false};

    // 当前帧时间戳(毫秒)
    std::atomic<int64_t> m_currentAudioPts{0};

    // 同步锁
    QMutex m_audioMutex;

    // 视频帧计数
    int               m_frameCount{0};
    SDL_AudioDeviceID m_audioDevice;
    int               m_syncThreshold{10};

    std::shared_ptr<SyncData> m_syncData{nullptr};

    QMutex             m_audioBufMutex;
    QQueue<QByteArray> m_audioBufferQueue;
    int                m_audioBufPos = 0;

    // 音频重采样相关
    SwrContext    *m_swrContext{nullptr};      // 音频重采样上下文
    AVSampleFormat m_audioOutSampleFmt;        // 输出音频采样格式
    int            m_audioOutSampleRate{0};    // 输出音频采样率
    int            m_audioOutChannels{0};      // 输出音频通道数
    int64_t        m_audioOutChannelLayout{0}; // 输出音频通道布局

    // 上一次音频源格式信息，用于检测格式变化
    AVSampleFormat m_lastSrcFormat{AV_SAMPLE_FMT_NONE};
    int            m_lastSrcSampleRate{0};
    int            m_lastSrcChannels{0};
    int64_t        m_lastSrcChannelLayout{0};
};

#endif // AUDIORENDERTHREAD_H
