#ifndef APPCONTEXT_H
#define APPCONTEXT_H

#include "constants.h"

#include <QObject>

/**
 * @brief 应用程序上下文单例类
 * 
 * AppContext类提供了对应用程序全局状态和资源的访问。
 * 采用单例模式实现，确保在整个应用程序中只有一个实例。
 */
class AppContext : public QObject
{
    Q_OBJECT
public:
    static AppContext *instance();

    const PlayState getPlayState() const { return m_playState; }
    void            setPlayState(const PlayState &state);

signals:
    void sigPlayStateChanged(PlayState state);

private:
    PlayState m_playState{PlayState::StoppedState};

private:
    explicit AppContext(QObject *parent = nullptr);
    ~AppContext() override = default;

    // 禁用拷贝和赋值
    AppContext(const AppContext &) = delete;
    AppContext &operator=(const AppContext &) = delete;

    static AppContext *s_instance;
};

#endif // APPCONTEXT_H
