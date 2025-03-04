#include "renderthread.h"
#include "../gui/sdlwidget.h"
#include "avframequeue.h"

#include <QDebug>
#include <QElapsedTimer>

extern "C" {
#include <SDL2/SDL.h>
#include <libavutil/time.h>
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
{}

RenderThread::~RenderThread()
{
    stopProcess();
    wait();
    closeRenderer();
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

    SDL_AudioSpec wanted_spec;
    wanted_spec.freq = sampleRate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = 1024;
    wanted_spec.callback = nullptr;
    wanted_spec.userdata = nullptr;

    m_audioDevice = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, NULL, 0);
    if (m_audioDevice == 0) {
        qWarning() << "SDL_OpenAudioDevice failed:" << SDL_GetError();
        return false;
    }

    SDL_PauseAudioDevice(m_audioDevice, 0);

    qInfo() << "音频渲染器初始化成功, 采样率:" << sampleRate << "声道数:" << channels;

    m_audioInitialized = true;
    return true;
}

void RenderThread::closeRenderer()
{
    // 清理视频渲染器资源
    m_videoInitialized = false;

    // 清理音频渲染器资源
    if (m_audioInitialized) {
        SDL_CloseAudioDevice(m_audioDevice);
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        m_audioInitialized = false;
    }

    m_frameCount = 0;
}

int64_t RenderThread::getCurrentVideoPts() const
{
    return m_currentVideoPts;
}

int64_t RenderThread::getCurrentAudioPts() const
{
    return m_currentAudioPts;
}

void RenderThread::process()
{
    // 首先处理视频帧
    if (m_videoInitialized && m_videoFrameQueue && !m_videoFrameQueue->isEmpty()) {
        AVFrame *videoFrame = m_videoFrameQueue->dequeueNoWait();
        if (videoFrame) {
            // 渲染视频帧
            if (renderVideoFrame(videoFrame)) {
                // 更新视频时间戳
                if (videoFrame->pts != AV_NOPTS_VALUE) {
                    m_currentVideoPts = videoFrame->pts;
                    emit frameRendered(m_currentVideoPts);
                }
            }

            // 释放帧
            av_frame_free(&videoFrame);
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
    msleep(5);
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

    QMutexLocker locker(&m_audioMutex);

    if (frame->pts != AV_NOPTS_VALUE) {
        m_currentAudioPts = frame->pts;
    }

    if (!m_audioInitialized) {
        qWarning() << "音频设备尚未初始化";
        return false;
    }

    int ret = SDL_QueueAudio(m_audioDevice, frame->data[0], frame->linesize[0]);
    if (ret < 0) {
        qWarning() << "SDL_QueueAudio failed:" << SDL_GetError();
        return false;
    }

    return true;
}

void RenderThread::cleanup()
{
    closeRenderer();
}
