#ifndef APPCONTEXT_H
#define APPCONTEXT_H

#include "appdata.h"
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

    // 播放状态
    const PlayState getPlayState() const { return m_playState; }
    void            setPlayState(const PlayState &state) { m_playState = state; }
    inline bool     isPlaying() { return m_playState == PlayState::PlayingState; }
    inline bool     isPauseed() { return m_playState == PlayState::PausedState; }
    inline bool     isStopped() { return m_playState == PlayState::StoppedState; }

    // 声音控制
    void setVoiceState(const VoiceState &state);
    void setVolume(int volume);
    bool isMute();

    // appData
    std::shared_ptr<AppData> getAppData() const { return m_appData; }

signals:
    void sigPlayStateChanged(PlayState state);

private:
    PlayState m_playState{PlayState::StoppedState};

    std::shared_ptr<AppData> m_appData;

private:
    explicit AppContext(QObject *parent = nullptr);
    ~AppContext();

    // 禁用拷贝和赋值
    AppContext(const AppContext &) = delete;
    AppContext &operator=(const AppContext &) = delete;
};

#endif // APPCONTEXT_H
