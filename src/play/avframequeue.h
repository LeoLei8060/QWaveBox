#ifndef AVFRAMEQUEUE_H
#define AVFRAMEQUEUE_H

#include <memory>
#include <QMutex>
#include <QWaitCondition>
#include <queue>

extern "C" {
#include <libavutil/frame.h>
}

/**
 * @brief 媒体帧队列类 - 用于存储解码线程产生的AVFrame帧
 */
class AVFrameQueue
{
public:
    explicit AVFrameQueue(int maxSize = 3000);
    ~AVFrameQueue();

    // 清空队列
    void clear();

    // 放入一个帧
    bool enqueue(AVFrame *frame);

    // 获取一个帧，不会阻塞，如果队列为空返回nullptr
    AVFrame *dequeueNoWait();

    // 获取一个帧，如果队列为空会阻塞等待
    AVFrame *dequeue(int timeoutMs = -1);

    AVFrame *front();

    AVFrame *pop();

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
    std::queue<AVFrame *> m_frames;   // 帧队列
    int                   m_maxSize;  // 最大队列大小
    mutable QMutex        m_mutex;    // 互斥锁
    QWaitCondition        m_notEmpty; // 非空条件变量
    QWaitCondition        m_notFull;  // 非满条件变量
    bool                  m_finished; // 是否结束标志
};

#endif // AVFRAMEQUEUE_H
