#include "avframequeue.h"

AVFrameQueue::AVFrameQueue(int maxSize)
    : m_maxSize(maxSize)
    , m_finished(false)
{}

AVFrameQueue::~AVFrameQueue()
{
    clear();
}

void AVFrameQueue::clear()
{
    QMutexLocker locker(&m_mutex);

    // 释放所有帧内存
    while (!m_frames.empty()) {
        AVFrame *frame = m_frames.front();
        m_frames.pop();
        av_frame_free(&frame);
    }

    m_finished = false;
}

bool AVFrameQueue::enqueue(AVFrame *frame)
{
    if (!frame) {
        return false;
    }

    QMutexLocker locker(&m_mutex);

    // 如果队列已满，等待直到有空间或超时
    while (m_frames.size() >= m_maxSize && !m_finished) {
        if (!m_notFull.wait(&m_mutex, 10)) {
            // 超时或被唤醒后检查是否已经终止
            if (m_finished) {
                return false;
            }
        }
    }

    // 如果队列已标记为结束状态，不再添加新帧
    if (m_finished) {
        av_frame_free(&frame);
        return false;
    }

    // 复制一份帧数据
    AVFrame *f = av_frame_alloc();
    av_frame_ref(f, frame);

    // 放入队列
    m_frames.push(f);

    // 通知等待的消费者线程
    m_notEmpty.wakeOne();

    return true;
}

AVFrame *AVFrameQueue::dequeueNoWait()
{
    QMutexLocker locker(&m_mutex);

    if (m_frames.empty()) {
        return nullptr;
    }

    AVFrame *frame = m_frames.front();
    m_frames.pop();

    // 通知等待的生产者线程
    m_notFull.wakeOne();

    return frame;
}

AVFrame *AVFrameQueue::dequeue(int timeoutMs)
{
    QMutexLocker locker(&m_mutex);

    // 队列为空时等待
    while (m_frames.empty()) {
        if (m_finished) {
            return nullptr;
        }

        if (timeoutMs < 0) {
            // 永久等待
            m_notEmpty.wait(&m_mutex);
        } else {
            // 带超时的等待
            if (!m_notEmpty.wait(&m_mutex, timeoutMs)) {
                return nullptr; // 超时
            }
        }

        // 被唤醒后再次检查队列是否为空
        if (m_frames.empty() && m_finished) {
            return nullptr;
        }
    }

    AVFrame *frame = m_frames.front();
    m_frames.pop();

    // 通知等待的生产者线程
    m_notFull.wakeOne();

    return frame;
}

AVFrame *AVFrameQueue::front()
{
    QMutexLocker locker(&m_mutex);
    return m_frames.front();
}

AVFrame *AVFrameQueue::pop()
{
    QMutexLocker locker(&m_mutex);
    AVFrame     *val = m_frames.front();
    m_frames.pop();
    return val;
}

int AVFrameQueue::size() const
{
    QMutexLocker locker(&m_mutex);
    return m_frames.size();
}

bool AVFrameQueue::isEmpty() const
{
    QMutexLocker locker(&m_mutex);
    return m_frames.empty();
}

bool AVFrameQueue::isFull() const
{
    QMutexLocker locker(&m_mutex);
    return m_frames.size() >= m_maxSize;
}

void AVFrameQueue::wakeUpAll()
{
    QMutexLocker locker(&m_mutex);
    m_notEmpty.wakeAll();
    m_notFull.wakeAll();
}

void AVFrameQueue::setFinished(bool finished)
{
    QMutexLocker locker(&m_mutex);
    m_finished = finished;

    if (finished) {
        // 唤醒所有等待的线程
        m_notEmpty.wakeAll();
        m_notFull.wakeAll();
    }
}

bool AVFrameQueue::isFinished() const
{
    QMutexLocker locker(&m_mutex);
    return m_finished;
}
