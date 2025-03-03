#include "threadbase.h"

ThreadBase::ThreadBase(QObject *parent)
    : QThread(parent)
{
}

ThreadBase::~ThreadBase()
{
    // 确保线程安全退出
    stopProcess();
    wait();
}

void ThreadBase::startProcess()
{
    m_running = true;
    m_paused = false;
    
    if (!isRunning()) {
        start();
    } else {
        // 如果线程已经在运行但处于暂停状态，恢复线程
        resumeProcess();
    }
}

void ThreadBase::pauseProcess()
{
    if (m_running && !m_paused) {
        QMutexLocker locker(&m_mutex);
        m_paused = true;
    }
}

void ThreadBase::resumeProcess()
{
    if (m_running && m_paused) {
        QMutexLocker locker(&m_mutex);
        m_paused = false;
        m_condition.wakeAll();
    }
}

void ThreadBase::stopProcess()
{
    m_running = false;
    
    // 如果正在暂停，需要唤醒线程以使其可以结束
    if (m_paused) {
        QMutexLocker locker(&m_mutex);
        m_paused = false;
        m_condition.wakeAll();
    }
}

bool ThreadBase::isRunning() const
{
    return QThread::isRunning() && m_running;
}

bool ThreadBase::isPaused() const
{
    return m_paused;
}

void ThreadBase::run()
{
    while (m_running) {
        // 处理暂停
        {
            QMutexLocker locker(&m_mutex);
            while (m_paused && m_running) {
                m_condition.wait(&m_mutex);
            }
        }
        
        // 如果已停止，退出循环
        if (!m_running) {
            break;
        }
        
        // 执行具体处理逻辑
        process();
    }
}
