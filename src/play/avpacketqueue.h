#ifndef AVPACKETQUEUE_H
#define AVPACKETQUEUE_H

#include <memory>
#include <QMutex>
#include <QWaitCondition>
#include <queue>

extern "C" {
#include <libavcodec/avcodec.h>
}

/**
 * @brief 媒体包队列类 - 用于存储解复用线程产生的AVPacket包
 */
class AVPacketQueue
{
public:
    explicit AVPacketQueue(int maxSize = 100);
    ~AVPacketQueue();

    // 清空队列
    void clear();

    // 放入一个包
    bool enqueue(AVPacket *packet);

    // 获取一个包，不会阻塞，如果队列为空返回nullptr
    AVPacket *dequeueNoWait();

    // 获取一个包，如果队列为空会阻塞等待
    AVPacket *dequeue(int timeoutMs = -1);

    // 获取队列当前大小
    int size() const;

    // 是否为空
    bool isEmpty() const;

    // 是否已满
    bool isFull() const;

    // 唤醒所有等待的线程
    void wakeUpAll();

    // 设置已结束标志（表示不会再有数据进入队列）
    void setFinished(bool finished = true);

    // 检查是否已结束
    bool isFinished() const;

private:
    std::queue<AVPacket *> m_packets;  // 包队列
    int                    m_maxSize;  // 最大队列大小
    mutable QMutex         m_mutex;    // 互斥锁
    QWaitCondition         m_notEmpty; // 非空条件变量
    QWaitCondition         m_notFull;  // 非满条件变量
    bool                   m_finished; // 是否结束标志
};

#endif // AVPACKETQUEUE_H
