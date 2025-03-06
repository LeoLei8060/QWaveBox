#include "audiorenderthread.h"
#include "avframequeue.h"

#include <QDebug>

extern "C" {
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>
}

AudioRenderThread::AudioRenderThread(QObject *parent)
    : ThreadBase{parent}
{
    // 确保 SDL 音频子系统已经初始化
    if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
        qDebug() << "初始化构造函数时 SDL 音频子系统未初始化";
    }
}

AudioRenderThread::~AudioRenderThread()
{
    qDebug() << "RenderThread 析构函数开始执行";

    // 停止线程
    stopProcess();

    // 确保线程已经退出
    if (isRunning()) {
        qDebug() << "等待线程结束...";
        wait();
    }

    // 关闭渲染器并释放资源
    closeRenderer();

    qDebug() << "RenderThread 析构函数执行完毕";
}

bool AudioRenderThread::initialize()
{
    // 渲染线程初始化
    return true;
}

void AudioRenderThread::setSyncData(std::shared_ptr<SyncData> data)
{
    m_syncData = data;
}

void AudioRenderThread::setAudioFrameQueue(AVFrameQueue *queue)
{
    m_audioFrameQueue = queue;
}

bool AudioRenderThread::initializeAudioRenderer(int sampleRate, int channels, int64_t channelLayout)
{
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        qWarning() << "SDL_Init(SDL_INIT_AUDIO) failed:" << SDL_GetError();
        return false;
    }

    // 保存音频参数
    m_audioOutSampleRate = sampleRate;
    m_audioOutChannels = channels;

    // 如果通道布局为0，根据通道数计算默认布局
    if (channelLayout == 0) {
        channelLayout = av_get_default_channel_layout(channels);
    }
    m_audioOutChannelLayout = channelLayout;

    qDebug() << "初始化音频渲染器 - 采样率:" << sampleRate << "通道数:" << channels
             << "通道布局:" << channelLayout;

    // 设置SDL音频规格
    SDL_AudioSpec wanted_spec;
    SDL_memset(&wanted_spec, 0, sizeof(wanted_spec));
    wanted_spec.freq = sampleRate;
    wanted_spec.format = AUDIO_S16SYS; // SDL需要S16格式，这是我们重采样的目标格式
    wanted_spec.channels = channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = 1024; // 缓冲区大小
    wanted_spec.callback = &AudioRenderThread::sdlAudioCallback;
    wanted_spec.userdata = this;

    // 实际获取的音频规格
    SDL_AudioSpec obtained_spec;

    // 打开音频设备，获取实际支持的规格
    m_audioDevice = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &obtained_spec, 0);
    if (m_audioDevice == 0) {
        qWarning() << "SDL_OpenAudioDevice failed:" << SDL_GetError();
        return false;
    }

    // 记录实际使用的音频参数
    m_audioOutSampleRate = obtained_spec.freq;
    m_audioOutChannels = obtained_spec.channels;
    m_audioOutSampleFmt = AV_SAMPLE_FMT_S16; // SDL使用S16格式

    // 根据实际通道数重新计算通道布局
    m_audioOutChannelLayout = av_get_default_channel_layout(obtained_spec.channels);

    qInfo() << "音频渲染器初始化成功 - 实际参数: 采样率:" << obtained_spec.freq
            << "通道数:" << obtained_spec.channels << "格式:" << obtained_spec.format
            << "缓冲区大小:" << obtained_spec.samples << "通道布局:" << m_audioOutChannelLayout;

    // 启动音频播放
    SDL_PauseAudioDevice(m_audioDevice, 0);

    m_audioInitialized = true;
    return true;
}

void AudioRenderThread::closeRenderer()
{
    qDebug() << "关闭渲染器，准备释放资源...";

    // 清理音频渲染器资源
    if (m_audioInitialized) {
        qDebug() << "释放音频资源";

        // 关闭SDL音频设备
        if (m_audioDevice != 0) {
            qDebug() << "关闭SDL音频设备: " << m_audioDevice;
            SDL_PauseAudioDevice(m_audioDevice, 1); // 暂停音频播放
            SDL_CloseAudioDevice(m_audioDevice);
            m_audioDevice = 0;
        }

        // 清理音频缓冲队列
        {
            QMutexLocker locker(&m_audioBufMutex);
            m_audioBufferQueue.clear();
            m_audioBufPos = 0;
        }

        // 释放重采样上下文
        if (m_swrContext) {
            qDebug() << "释放音频重采样上下文";
            swr_free(&m_swrContext);
            m_swrContext = nullptr;
        }

        // 重置音频参数
        m_lastSrcFormat = AV_SAMPLE_FMT_NONE;
        m_lastSrcSampleRate = 0;
        m_lastSrcChannels = 0;
        m_lastSrcChannelLayout = 0;

        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        m_audioInitialized = false;
    }

    m_frameCount = 0;
    qDebug() << "渲染器资源释放完毕";
}

int64_t AudioRenderThread::getCurrentAudioPts() const
{
    return m_currentAudioPts;
}

void AudioRenderThread::process()
{
    AVFrame *audioFrame = nullptr;

    // 获取视频和音频帧
    if (m_audioInitialized && m_audioFrameQueue && !m_audioFrameQueue->isEmpty())
        audioFrame = m_audioFrameQueue->dequeueNoWait();

    // 测试
    // if (audioFrame) {
    //     static int s_num = -1;
    //     if (audioFrame->pts / 44100 == s_num + 1) {
    //         qDebug() << "audio: " << audioFrame->pts << audioFrame->pts * 0.02267;
    //         s_num++;
    //     }
    //     av_frame_free(&audioFrame);
    // }
    // return;

    // 先更新时间戳
    // if (audioFrame) {
    //     m_syncData->setAudioPts(audioFrame->pts);
    // }

    // 然后处理音频帧
    if (audioFrame) {
        // 渲染音频帧
        renderAudioFrame(audioFrame);

        // 释放帧
        av_frame_free(&audioFrame);
    } /*else {
        msleep(1);
        qDebug() << "audioFrame is Empty";
    }*/

    // 检查是否都已结束
    if (m_audioFrameQueue && m_audioFrameQueue->isFinished() && m_audioFrameQueue->isEmpty()) {
        // 暂停线程，等待重新开始
        pauseProcess();
    }

    // 控制渲染速率，避免CPU占用过高
    msleep(15); // 减少睡眠时间，提高处理响应性
}

bool AudioRenderThread::renderAudioFrame(AVFrame *frame)
{
    if (!frame) {
        return false;
    }

    QMutexLocker locker(&m_audioBufMutex);

    // 获取音频帧的基本信息
    AVSampleFormat srcFormat = static_cast<AVSampleFormat>(frame->format);
    int            srcSampleRate = frame->sample_rate;
    int            srcChannels = frame->channels;
    int64_t        srcChannelLayout = frame->channel_layout;

    // 如果通道布局未设置，根据通道数计算默认布局
    if (srcChannelLayout == 0) {
        srcChannelLayout = av_get_default_channel_layout(srcChannels);
    }

    // 检查音频帧是否需要转换
    bool needResample = (srcFormat != AV_SAMPLE_FMT_S16) || (srcSampleRate != m_audioOutSampleRate)
                        || (srcChannels != m_audioOutChannels);

    // 检查输出通道布局是否合法
    if (m_audioOutChannelLayout == 0 && m_audioOutChannels > 0) {
        m_audioOutChannelLayout = av_get_default_channel_layout(m_audioOutChannels);
    }

    QByteArray audioData;

    if (needResample) {
        // 如果音频参数发生变化，需要重新创建重采样上下文
        if (!m_swrContext || srcFormat != m_lastSrcFormat || srcSampleRate != m_lastSrcSampleRate
            || srcChannels != m_lastSrcChannels || srcChannelLayout != m_lastSrcChannelLayout) {
            // 释放旧的上下文
            if (m_swrContext) {
                swr_free(&m_swrContext);
                m_swrContext = nullptr;
            }

            // 使用便捷函数创建重采样上下文
            m_swrContext = swr_alloc_set_opts(nullptr,
                                              m_audioOutChannelLayout, // 输出通道布局
                                              AV_SAMPLE_FMT_S16,       // 输出格式 (SDL兼容)
                                              m_audioOutSampleRate,    // 输出采样率
                                              srcChannelLayout,        // 输入通道布局
                                              srcFormat,               // 输入格式
                                              srcSampleRate,           // 输入采样率
                                              0,
                                              nullptr);

            if (!m_swrContext) {
                qWarning() << "创建音频重采样上下文失败";
                return false;
            }

            // 初始化重采样上下文
            int ret = swr_init(m_swrContext);
            if (ret < 0) {
                char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
                qWarning() << "初始化音频重采样上下文失败: " << errbuf;
                swr_free(&m_swrContext);
                m_swrContext = nullptr;
                return false;
            }

            // 保存当前源格式参数，用于下次比较
            m_lastSrcFormat = srcFormat;
            m_lastSrcSampleRate = srcSampleRate;
            m_lastSrcChannels = srcChannels;
            m_lastSrcChannelLayout = srcChannelLayout;
        }

        // 计算输出样本数量 (考虑采样率变化)
        int outSamples = av_rescale_rnd(frame->nb_samples,
                                        m_audioOutSampleRate,
                                        srcSampleRate,
                                        AV_ROUND_UP);

        // 临时空间用于存放转换后的数据
        uint8_t *outBuffer = nullptr;
        int      outLinesize;
        int      ret = av_samples_alloc(&outBuffer,
                                   &outLinesize,
                                   m_audioOutChannels,
                                   outSamples,
                                   AV_SAMPLE_FMT_S16,
                                   0);

        if (ret < 0) {
            qWarning() << "分配重采样缓冲区失败";
            return false;
        }

        // 执行实际的重采样
        outSamples = swr_convert(m_swrContext,
                                 &outBuffer,
                                 outSamples,
                                 (const uint8_t **) frame->data,
                                 frame->nb_samples);

        if (outSamples <= 0) {
            qWarning() << "音频重采样失败: " << outSamples;
            av_freep(&outBuffer);
            return false;
        }

        // 计算重采样后的数据大小 (字节数)
        int dataSize = av_samples_get_buffer_size(nullptr,
                                                  m_audioOutChannels,
                                                  outSamples,
                                                  AV_SAMPLE_FMT_S16,
                                                  1);

        if (dataSize <= 0) {
            qWarning() << "获取重采样后的缓冲区大小失败";
            av_freep(&outBuffer);
            return false;
        }

        // 复制数据到QByteArray
        audioData = QByteArray((const char *) outBuffer, dataSize);

        // 释放临时缓冲区
        av_freep(&outBuffer);
    } else {
        // 不需要重采样，直接使用原始数据
        int dataSize = av_samples_get_buffer_size(nullptr,
                                                  srcChannels,
                                                  frame->nb_samples,
                                                  srcFormat,
                                                  1);

        if (dataSize <= 0) {
            qWarning() << "获取原始音频数据大小失败";
            return false;
        }

        // 复制音频数据到QByteArray
        audioData = QByteArray((const char *) frame->data[0], dataSize);
    }

    // 将处理后的音频数据放入队列
    if (!audioData.isEmpty()) {
        m_audioBufferQueue.enqueue(audioData);
    } else {
        qWarning() << "处理后的音频数据为空";
    }

    // 更新音频时间戳到SyncData
    if (frame->pts != AV_NOPTS_VALUE) {
        // 已经在process()方法中更新时间戳，此处不需要重复设置
        // m_currentAudioPts = frame->pts;
        if (m_syncData) {
            m_syncData->setAudioData(frame->pts,
                                     /*frame->time_base.num*/ 1,
                                     /*frame->time_base.den*/ 44100);
            // qDebug() << "audio set: " << frame->pts;
        }
    }

    return true;
}

void AudioRenderThread::cleanup()
{
    closeRenderer();
}

void AudioRenderThread::audioCallback(Uint8 *stream, int len)
{
    QMutexLocker locker(&m_audioBufMutex);

    int offset = 0;
    while (offset < len) {
        // 缓冲区为空时填充静音
        if (m_audioBufferQueue.isEmpty()) {
            qDebug() << __FUNCTION__ << "audioBufferQueue is Empty.";
            memset(stream + offset, 0, len - offset);
            return;
        }

        QByteArray &chunk = m_audioBufferQueue.head();
        int         chunkSize = chunk.size() - m_audioBufPos;
        int         bytesNeeded = len - offset;

        // 计算本次可复制的字节数
        int bytesToCopy = qMin(bytesNeeded, chunkSize);
        memcpy(stream + offset, chunk.constData() + m_audioBufPos, bytesToCopy);

        offset += bytesToCopy;
        m_audioBufPos += bytesToCopy;

        // 当前chunk已消费完
        if (m_audioBufPos >= chunk.size()) {
            m_audioBufferQueue.dequeue();
            m_audioBufPos = 0;
        }
    }
}

void AudioRenderThread::sdlAudioCallback(void *userdata, Uint8 *stream, int len)
{
    // 转换userdata为RenderThread实例
    AudioRenderThread *self = static_cast<AudioRenderThread *>(userdata);
    if (self) {
        // 调用实例方法处理音频回调
        self->audioCallback(stream, len);
    } else {
        // 如果实例无效，填充静音
        memset(stream, 0, len);
    }
}
