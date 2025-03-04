#include "renderthread.h"
#include "../gui/sdlwidget.h"
#include "avframequeue.h"

#include <QDebug>
#include <QElapsedTimer>

extern "C" {
#include <SDL2/SDL.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>
}

RenderThread::RenderThread(QObject *parent)
    : ThreadBase(parent)
    , m_videoFrameQueue(nullptr)
    , m_audioFrameQueue(nullptr)
    , m_videoWidget(nullptr)
    , m_videoInitialized(false)
    , m_audioInitialized(false)
    , m_currentVideoPts(0)
    , m_currentAudioPts(0)
    , m_frameCount(0)
    , m_audioDevice(0)
    , m_swrContext(nullptr)
    , m_audioOutSampleFmt(AV_SAMPLE_FMT_S16)
    , m_audioOutSampleRate(0)
    , m_audioOutChannels(0)
    , m_audioOutChannelLayout(0)
    , m_lastSrcFormat(AV_SAMPLE_FMT_NONE)
    , m_lastSrcSampleRate(0)
    , m_lastSrcChannels(0)
    , m_lastSrcChannelLayout(0)
    , m_audioBufPos(0)
    , m_syncData(nullptr)
{
    // 确保 SDL 音频子系统已经初始化
    if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
        qDebug() << "初始化构造函数时 SDL 音频子系统未初始化";
    }
}

RenderThread::~RenderThread()
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

bool RenderThread::initialize()
{
    // 渲染线程初始化
    return true;
}

void RenderThread::setSyncData(std::shared_ptr<SyncData> data)
{
    m_syncData = data;
}

void RenderThread::setVideoFrameQueue(AVFrameQueue *queue)
{
    m_videoFrameQueue = queue;
}

void RenderThread::setAudioFrameQueue(AVFrameQueue *queue)
{
    m_audioFrameQueue = queue;
}

void RenderThread::setVideoWidget(SDLWidget *widget)
{
    m_videoWidget = widget;
}

bool RenderThread::initializeVideoRenderer()
{
    if (!m_videoWidget) {
        qWarning() << "无效的视频渲染控件";
        return false;
    }

    // 初始化SDL渲染器
    if (!m_videoWidget->initializeSDL()) {
        qWarning() << "初始化SDL渲染器失败";
        return false;
    }

    m_videoInitialized = true;
    qInfo() << "视频渲染器初始化成功";
    return true;
}

bool RenderThread::initializeAudioRenderer(int sampleRate, int channels, int64_t channelLayout)
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
    wanted_spec.callback = &RenderThread::sdlAudioCallback;
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

void RenderThread::closeRenderer()
{
    qDebug() << "关闭渲染器，准备释放资源...";

    // 清理视频渲染器资源
    m_videoInitialized = false;

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

int64_t RenderThread::getCurrentVideoPts() const
{
    return m_currentVideoPts;
}

int64_t RenderThread::getCurrentAudioPts() const
{
    return m_currentAudioPts;
}

void RenderThread::onSyncEvent(int64_t masterClock, double videoDelay, double audioDelay)
{
    qDebug() << "Sync event: masterClock=" << masterClock << "videoDelay=" << videoDelay
             << "audioDelay=" << audioDelay;
}

void RenderThread::process()
{
    static int frameCounter = 0;
    static int droppedFrames = 0;

    // 首先处理视频帧
    if (m_videoInitialized && m_videoFrameQueue && !m_videoFrameQueue->isEmpty()) {
        for (int i = 0; i < 2; ++i) {
            AVFrame *videoFrame = m_videoFrameQueue->dequeueNoWait();
            if (videoFrame) {
                frameCounter++;
                // 获取视频时间戳
                int64_t videoPts = AV_NOPTS_VALUE;
                if (videoFrame->pts != AV_NOPTS_VALUE) {
                    videoPts = videoFrame->pts;
                }

                // 应用同步逻辑
                bool renderFrame = true;
                if (m_syncData && videoPts != AV_NOPTS_VALUE) {
                    // 获取同步信息
                    int64_t masterClock = m_syncData->getMasterClock();

                    // 检查是否需要延迟播放视频帧
                    int64_t diff = videoPts - masterClock;

                    // 只输出每10帧的同步信息，避免日志过多
                    if (frameCounter % 10 == 0) {
                        qDebug() << "视频同步状态 - 视频PTS:" << videoPts
                                 << "主时钟:" << masterClock << "差异:" << diff << "ms"
                                 << "已处理帧数:" << frameCounter << "丢弃帧数:" << droppedFrames;
                    }

                    if (diff > 100) {
                        // 视频帧时间戳大于主时钟太多，需要等待
                        int waitTime = static_cast<int>(diff);
                        if (waitTime > 1000) {
                            // 差异过大，直接跳过该帧
                            qDebug() << "视频帧时间戳差异过大，跳过该帧: " << diff << "ms";
                            renderFrame = false;
                            droppedFrames++;
                        } else if (waitTime > 5) {
                            // 等待一段时间使视频与音频同步
                            qDebug() << "视频延迟等待: " << waitTime << "ms";
                            msleep(waitTime);
                        }
                    } else if (diff < -500) { // 增加落后容忍度，只有落后超过200ms才跳过
                        // 视频帧时间戳小于主时钟太多，视频太慢了，需要跳过该帧加速视频
                        qDebug() << "视频帧严重落后，跳过该帧: " << -diff << "ms" << videoPts
                                 << masterClock;
                        renderFrame = false;
                        droppedFrames++;
                    }
                    // 否则就正常渲染，允许视频有一定程度的落后

                    // 更新同步数据
                    m_syncData->setVideoPts(videoPts);
                }

                // 基于同步决定是否渲染视频帧
                if (renderFrame) {
                    // 渲染视频帧
                    if (renderVideoFrame(videoFrame)) {
                        // 更新视频时间戳
                        if (videoPts != AV_NOPTS_VALUE) {
                            m_currentVideoPts = videoPts;
                            emit frameRendered(m_currentVideoPts);
                        }
                    }
                }

                // 释放帧
                av_frame_free(&videoFrame);

                if (renderFrame)
                    break;
            }
        }
    }

    // 然后处理音频帧
    if (m_audioInitialized && m_audioFrameQueue && !m_audioFrameQueue->isEmpty()) {
        AVFrame *audioFrame = m_audioFrameQueue->dequeueNoWait();
        if (audioFrame) {
            // 渲染音频帧
            if (renderAudioFrame(audioFrame)) {
                // 更新音频时间戳
                if (audioFrame->pts != AV_NOPTS_VALUE) {
                    m_currentAudioPts = audioFrame->pts;

                    // 更新同步数据，音频为主时钟
                    if (m_syncData) {
                        m_syncData->setAudioPts(m_currentAudioPts);
                        m_syncData->setMasterClock(m_currentAudioPts);
                    }
                }
            }

            // 释放帧
            av_frame_free(&audioFrame);
        }
    }

    // 检查是否都已结束
    if ((m_videoFrameQueue && m_videoFrameQueue->isFinished() && m_videoFrameQueue->isEmpty())
        && (m_audioFrameQueue && m_audioFrameQueue->isFinished() && m_audioFrameQueue->isEmpty())) {
        // 发出渲染完成信号
        emit renderFinished();

        // 暂停线程，等待重新开始
        pauseProcess();
    }

    // 控制渲染速率，避免CPU占用过高
    msleep(1); // 减少睡眠时间，提高处理响应性
}

bool RenderThread::renderVideoFrame(AVFrame *frame)
{
    if (!m_videoWidget || !frame) {
        return false;
    }

    QMutexLocker locker(&m_videoMutex);

    try {
        // 渲染帧
        m_videoWidget->renderFrame(frame);
        m_frameCount++;

        // 更新视频时间戳到 SyncData
        if (frame->pts != AV_NOPTS_VALUE) {
            m_currentVideoPts = frame->pts;
            if (m_syncData) {
                m_syncData->setVideoPts(m_currentVideoPts);
            }
            emit frameRendered(m_currentVideoPts);
        }

        // 每100帧输出一次日志
        if (m_frameCount % 100 == 0) {
            qInfo() << "已渲染" << m_frameCount << "帧";
        }

        return true;
    } catch (const std::exception &e) {
        qWarning() << "渲染视频帧出错:" << e.what();
        return false;
    }
}

bool RenderThread::renderAudioFrame(AVFrame *frame)
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
        m_currentAudioPts = frame->pts;
        if (m_syncData) {
            m_syncData->setAudioPts(m_currentAudioPts);
        }
    }

    return true;
}

void RenderThread::cleanup()
{
    closeRenderer();
}

void RenderThread::audioCallback(Uint8 *stream, int len)
{
    QMutexLocker locker(&m_audioBufMutex);

    int offset = 0;
    while (offset < len) {
        // 缓冲区为空时填充静音
        if (m_audioBufferQueue.isEmpty()) {
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

void RenderThread::sdlAudioCallback(void *userdata, Uint8 *stream, int len)
{
    // 转换userdata为RenderThread实例
    RenderThread *self = static_cast<RenderThread *>(userdata);
    if (self) {
        // 调用实例方法处理音频回调
        self->audioCallback(stream, len);
    } else {
        // 如果实例无效，填充静音
        memset(stream, 0, len);
    }
}
