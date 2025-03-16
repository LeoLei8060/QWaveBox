#ifndef CONSTANTS_H
#define CONSTANTS_H

enum class PlayState {
    StoppedState = 0, // 暂停状态（包含未播放状态）
    PlayingState,     // 播放状态
    PausedState       // 暂停状态
};

enum class VoiceState {
    NormalState = 0, // 正常状态
    MuteState        // 静音状态
};

enum HotkeyType {
    K_OpenFile = 1, // 打开文件（F3)
    K_OpenFolder,   // 打开文件夹（F2)
    K_Close,        // 关闭（F4)
    K_Options,      // 选项（F5)
    K_About,        // 关于（F1)
    K_Quit,         // 退出（Ctrl+F4）
    K_PlaySelected, // 播放选中项（Enter）
    K_PausePlay,    // 暂停/继续播放（Space）
    K_NextPlay,     // 快进/下一个（Right）
    K_PrevPlay      // 快退/上一个（Left）
};

#endif // CONSTANTS_H
