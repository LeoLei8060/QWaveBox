#include "appcontext.h"

// 初始化静态成员变量
AppContext *AppContext::s_instance = nullptr;

AppContext *AppContext::instance()
{
    if (!s_instance) {
        s_instance = new AppContext();
    }
    return s_instance;
}

// void AppContext::setPlayState(const PlayState &state)
// {
//     if (m_playState == state)
//         return;
//     m_playState = state;
//     emit sigPlayStateChanged(m_playState);
// }

AppContext::AppContext(QObject *parent)
    : QObject(parent)
{}
