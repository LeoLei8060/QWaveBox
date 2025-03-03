#include "avpacketqueue.h"

AVPacketQueue::AVPacketQueue(int maxSize)
    : m_maxSize(maxSize)
    , m_finished(false)
{
}

AVPacketQueue::~AVPacketQueue()
{
    clear();
}

void AVPacketQueue::clear()
{
    QMutexLocker locker(&m_mutex);
    
    // 释放所有包内存
    while (!m_packets.empty()) {
        AVPacket *packet = m_packets.front();
        m_packets.pop();
        av_packet_free(&packet);
    }
    
    m_finished = false;
}

bool AVPacketQueue::enqueue(AVPacket *packet)
{
    if (!packet) {
        return false;
    }
    
    QMutexLocker locker(&m_mutex);
    
    // 如果队列已满，等待直到有空间或超时
    while (m_packets.size() >= m_maxSize && !m_finished) {
        if (!m_notFull.wait(&m_mutex, 10)) {
            // 超时或被唤醒后检查是否已经终止
            if (m_finished) {
                return false;
            }
        }
    }
    
    // 如果队列已标记为结束状态，不再添加新包
    if (m_finished) {
        av_packet_free(&packet);
        return false;
    }
    
    // 复制一份包数据
    AVPacket *pkt = av_packet_alloc();
    av_packet_ref(pkt, packet);
    
    // 放入队列
    m_packets.push(pkt);
    
    // 通知等待的消费者线程
    m_notEmpty.wakeOne();
    
    return true;
}

AVPacket* AVPacketQueue::dequeueNoWait()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_packets.empty()) {
        return nullptr;
    }
    
    AVPacket *packet = m_packets.front();
    m_packets.pop();
    
    // 通知等待的生产者线程
    m_notFull.wakeOne();
    
    return packet;
}

AVPacket* AVPacketQueue::dequeue(int timeoutMs)
{
    QMutexLocker locker(&m_mutex);
    
    // 队列为空时等待
    while (m_packets.empty()) {
        if (m_finished) {
            return nullptr;
        }
        
        if (timeoutMs < 0) {
            // 永久等待
            m_notEmpty.wait(&m_mutex);
        } else {
            // 带超时的等待
            if (!m_notEmpty.wait(&m_mutex, timeoutMs)) {
                return nullptr;  // 超时
            }
        }
        
        // 被唤醒后再次检查队列是否为空
        if (m_packets.empty() && m_finished) {
            return nullptr;
        }
    }
    
    AVPacket *packet = m_packets.front();
    m_packets.pop();
    
    // 通知等待的生产者线程
    m_notFull.wakeOne();
    
    return packet;
}

int AVPacketQueue::size() const
{
    QMutexLocker locker(&m_mutex);
    return m_packets.size();
}

bool AVPacketQueue::isEmpty() const
{
    QMutexLocker locker(&m_mutex);
    return m_packets.empty();
}

bool AVPacketQueue::isFull() const
{
    QMutexLocker locker(&m_mutex);
    return m_packets.size() >= m_maxSize;
}

void AVPacketQueue::wakeUpAll()
{
    QMutexLocker locker(&m_mutex);
    m_notEmpty.wakeAll();
    m_notFull.wakeAll();
}

void AVPacketQueue::setFinished(bool finished)
{
    QMutexLocker locker(&m_mutex);
    m_finished = finished;
    
    if (finished) {
        // 唤醒所有等待的线程
        m_notEmpty.wakeAll();
        m_notFull.wakeAll();
    }
}

bool AVPacketQueue::isFinished() const
{
    QMutexLocker locker(&m_mutex);
    return m_finished;
}
