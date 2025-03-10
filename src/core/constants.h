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

#endif // CONSTANTS_H
