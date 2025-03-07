#include "audiorenderthread.h"
#include "avframequeue.h"

#include <QDebug>

extern "C" {
#include <libavutil/opt.h>
#include <libavutil/time.h>
}

AudioRenderThread::AudioRenderThread(QObject *parent)
    : ThreadBase{parent}
{}

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

void AudioRenderThread::setAudioFrameQueue(AVFrameQueue *queue)
{
    m_audioFrameQueue = queue;
}

bool AudioRenderThread::initializeAudioRenderer(AVSync            *sync,
                                                AVRational         timebase,
                                                AVCodecParameters *audioParams)
{
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        qWarning() << "SDL_Init(SDL_INIT_AUDIO) failed:" << SDL_GetError();
        return false;
    }

    m_avSync = sync;
    m_timebase = timebase;
    m_codecpar = audioParams;
    // 转换：
    m_inParams.sample_rate_ = audioParams->sample_rate;
    m_inParams.channels_ = audioParams->channels;
    m_inParams.channel_layout_ = audioParams->channel_layout;
    m_inParams.fmt_ = (AVSampleFormat) audioParams->format;
    m_inParams.frame_size_ = audioParams->frame_size;

    // 设置SDL音频规格
    SDL_AudioSpec wanted_spec;
    SDL_memset(&wanted_spec, 0, sizeof(wanted_spec));
    wanted_spec.freq = m_inParams.sample_rate_;
    wanted_spec.format = AUDIO_S16SYS; // SDL需要S16格式，这是我们重采样的目标格式
    wanted_spec.channels = m_inParams.channels_;
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
    m_outParams.sample_rate_ = wanted_spec.freq;
    m_outParams.channels_ = wanted_spec.channels;
    m_outParams.channel_layout_ = av_get_default_channel_layout(2);
    m_outParams.fmt_ = AV_SAMPLE_FMT_S16;
    m_outParams.frame_size_ = 1024 /*obtained_spec.samples*/;

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

        // 释放重采样上下文
        if (m_swrContext) {
            qDebug() << "释放音频重采样上下文";
            swr_free(&m_swrContext);
            m_swrContext = nullptr;
        }

        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        m_audioInitialized = false;
    }

    qDebug() << "渲染器资源释放完毕";
}

void AudioRenderThread::setVolume(int volume)
{
    double val = volume / 100.0;
    m_volume = val;
}

void AudioRenderThread::process()
{
    // 采用回调函数的方式播放，不需要线程处理业务
    // TODO: 待优化
    msleep(10);
}

void AudioRenderThread::cleanup()
{
    closeRenderer();
}

void AudioRenderThread::audioCallback(Uint8 *stream, int len)
{
    if (!m_audioFrameQueue) {
        qWarning() << "err: audioFrameQueue is null";
        return;
    }
    uint32_t bufIndex = 0, bufSize = 0, buf1Size = 0;
    uint8_t *buf = nullptr, *buf1 = nullptr;
    int64_t  pts = AV_NOPTS_VALUE;
    int      len1 = 0;

    while (len > 0) { // >0 表示有数据未处理
        if (bufIndex == bufSize) {
            bufIndex = 0;
            AVFrame *frame = m_audioFrameQueue->dequeueNoWait();
            if (frame) {
                pts = frame->pts;

                if ((!m_swrContext)
                    && ((frame->format != m_outParams.fmt_)
                        || (frame->sample_rate != m_outParams.sample_rate_)
                        || (frame->channel_layout != m_outParams.channel_layout_))) {
                    m_swrContext = swr_alloc_set_opts(NULL,
                                                      m_outParams.channel_layout_,
                                                      (AVSampleFormat) m_outParams.fmt_,
                                                      m_outParams.sample_rate_,
                                                      frame->channel_layout,
                                                      (AVSampleFormat) frame->format,
                                                      frame->sample_rate,
                                                      0,
                                                      NULL);
                    if (!m_swrContext || swr_init(m_swrContext) < 0) {
                        qWarning() << "create sample rate converter failed.";
                        swr_free((SwrContext **) (&m_swrContext));
                        return;
                    }
                }
                if (m_swrContext) { // 重采样
                    const uint8_t **inBuf = (const uint8_t **) frame->extended_data;
                    uint8_t       **outBuf = &buf1;
                    int             outSamples = frame->nb_samples * m_outParams.sample_rate_
                                         / frame->sample_rate
                                     + 256;
                    int outBytes = av_samples_get_buffer_size(NULL,
                                                              m_outParams.channels_,
                                                              outSamples,
                                                              m_outParams.fmt_,
                                                              0);
                    if (outBytes < 0) {
                        qWarning() << "av_samples_get_buffer_size failed.";
                        return;
                    }
                    av_fast_malloc(&buf1, &buf1Size, outBytes);

                    int len2 = swr_convert(m_swrContext,
                                           outBuf,
                                           outSamples,
                                           inBuf,
                                           frame->nb_samples); // 返回样本数量
                    if (len2 < 0) {
                        qWarning() << "swr_convert failed.";
                        return;
                    }
                    buf = buf1;
                    bufSize = av_samples_get_buffer_size(NULL,
                                                         m_outParams.channels_,
                                                         len2,
                                                         (AVSampleFormat) m_outParams.fmt_,
                                                         1);
                } else { // 没有重采样
                    int size = av_samples_get_buffer_size(NULL,
                                                          frame->channels,
                                                          frame->nb_samples,
                                                          (AVSampleFormat) frame->format,
                                                          1);
                    av_fast_malloc(&buf1, &buf1Size, size);
                    buf = buf1;
                    bufSize = size;
                    memcpy(buf, frame->data[0], size);
                }
                av_frame_free(&frame);
            } else {
                buf = nullptr;
                bufSize = 512;
            }
        }
        len1 = bufSize - bufIndex;
        if (len1 > len)
            len1 = len;
        if (!buf)
            memset(stream, 0, len1);
        else
            memcpy(stream, buf + bufIndex, len1);
        len -= len1;
        stream += len1;
        bufIndex += len1;
    }

    // 更新时钟
    if (pts != AV_NOPTS_VALUE) {
        double tm = pts * av_q2d(m_timebase);
        m_avSync->setClock(tm);
    }
}

void AudioRenderThread::sdlAudioCallback(void *userdata, Uint8 *stream, int len)
{
    // 转换userdata为RenderThread实例
    AudioRenderThread *self = static_cast<AudioRenderThread *>(userdata);
    if (self) {
        // 调用实例方法处理音频回调
        self->audioCallback(stream, len);

        if (self->m_volume != 1.0f) { // 避免不必要的计算
            Sint16 *samples = reinterpret_cast<Sint16 *>(stream);
            int     numSamples = len / sizeof(Sint16);
            for (int i = 0; i < numSamples; ++i) {
                samples[i] = static_cast<Sint16>(samples[i] * self->m_volume);
            }
        }

    } else {
        // 如果实例无效，填充静音
        memset(stream, 0, len);
    }
}
