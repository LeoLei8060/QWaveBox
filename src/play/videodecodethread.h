#ifndef VIDEODECODETHREAD_H
#define VIDEODECODETHREAD_H

#include "threadbase.h"
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
}

class AVPacketQueue;
class AVFrameQueue;

/**
 * @brief 视频解码线程类 - 负责将视频包解码为视频帧
 */
class VideoDecodeThread : public ThreadBase
{
    Q_OBJECT
public:
    explicit VideoDecodeThread(QObject *parent = nullptr);
    ~VideoDecodeThread() override;

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

signals:
    // 解码完成信号
    void decodeFinished();

    // TODO: 待删除
    void sigSendAVFrame(AVFrame *);

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

#endif // VIDEODECODETHREAD_H
