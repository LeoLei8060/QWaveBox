#ifndef AUDIORENDERTHREAD_H
#define AUDIORENDERTHREAD_H

#include "avsync.h"
#include "threadbase.h"

#include <memory>
#include <QMutex>
#include <QQueue>

extern "C" {
#include <SDL.h>
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

class AVFrameQueue;

struct AudioParams
{
    int            sample_rate_;
    int            channels_;
    int64_t        channel_layout_;
    AVSampleFormat fmt_;
    int            frame_size_;
};

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

    // 设置音频帧队列
    void setAudioFrameQueue(AVFrameQueue *queue);

    // 初始化音频渲染器
    bool initializeAudioRenderer(AVRational timebase, AVCodecParameters *audioParams);

    // 绑定同步时钟
    void setSync(AVSync *sync);

    // 关闭渲染器
    void closeRenderer();

    void pausePlay();
    void resumePlay();

    void setVolume(int volume);

protected:
    // 线程处理函数
    void process() override;

private:
    // 清理资源
    void cleanup();

    // 回调
    void        audioCallback(Uint8 *stream, int len);
    static void sdlAudioCallback(void *userdata, Uint8 *stream, int len);

private:
    // 帧队列
    AVFrameQueue *m_audioFrameQueue{nullptr};

    bool m_audioInitialized{false};

    SDL_AudioDeviceID  m_audioDevice;
    SwrContext        *m_swrContext{nullptr}; // 音频重采样上下文
    AVSync            *m_avSync = nullptr;
    AVRational         m_timebase;
    AVCodecParameters *m_codecpar = nullptr;
    AudioParams        m_inParams;
    AudioParams        m_outParams;

    double m_volume = 1.0;
};

#endif // AUDIORENDERTHREAD_H
