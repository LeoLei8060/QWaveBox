#ifndef AUDIODECODETHREAD_H
#define AUDIODECODETHREAD_H

#include "threadbase.h"
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
}

class AVPacketQueue;
class AVFrameQueue;

/**
 * @brief 音频解码线程类 - 负责将音频包解码为音频帧
 */
class AudioDecodeThread : public ThreadBase
{
    Q_OBJECT
public:
    explicit AudioDecodeThread(QObject *parent = nullptr);
    ~AudioDecodeThread() override;

    // 初始化线程
    bool initialize() override;

    // 设置输入包队列
    void setPacketQueue(AVPacketQueue *queue);

    // 获取输出帧队列
    AVFrameQueue *getFrameQueue() const;

    // 打开解码器
    bool openDecoder(int streamIndex, AVCodecParameters *codecParams);

    // 关闭解码器
    void closeDecoder();

    // 清空解码缓冲区
    void flush();

    // 获取音频参数
    int            getSampleRate() const;
    int            getChannels() const;
    AVSampleFormat getSampleFormat() const;
    int64_t        getChannelLayout() const;

signals:
    // 解码完成信号
    void decodeFinished();

protected:
    // 线程处理函数
    void process() override;

private:
    // 解码一个包
    bool decodePacket(AVPacket *packet);

    // 清理资源
    void cleanup();

private:
    // 解码器相关
    AVCodecContext *m_codecContext{nullptr};

    // 输入包队列
    AVPacketQueue *m_packetQueue{nullptr};

    // 输出帧队列
    std::unique_ptr<AVFrameQueue> m_frameQueue;

    // 流索引
    int m_streamIndex{-1};
};

#endif // AUDIODECODETHREAD_H
