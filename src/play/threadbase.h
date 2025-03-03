#ifndef THREADBASE_H
#define THREADBASE_H

#include <atomic>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>

/**
 * @brief 线程基类 - 所有特定功能线程的基类
 */
class ThreadBase : public QThread
{
    Q_OBJECT
public:
    explicit ThreadBase(QObject *parent = nullptr);
    virtual ~ThreadBase();

    // 初始化线程
    virtual bool initialize() = 0;

    // 开始线程处理
    virtual void startProcess();

    // 暂停线程处理
    virtual void pauseProcess();

    // 恢复线程处理
    virtual void resumeProcess();

    // 停止线程处理
    virtual void stopProcess();

    // 检查线程是否正在运行
    bool isRunning() const;

    // 检查线程是否已暂停
    bool isPaused() const;

protected:
    // 线程执行函数
    virtual void run() override;

    // 具体的处理逻辑，由子类实现
    virtual void process() = 0;

signals:
    // 线程错误信号
    void threadError(const QString &errorMsg);

protected:
    // 线程控制变量
    std::atomic<bool> m_running{false}; // 线程是否应该运行
    std::atomic<bool> m_paused{false};  // 线程是否暂停

    // 线程同步
    QMutex         m_mutex;
    QWaitCondition m_condition;
};

#endif // THREADBASE_H
