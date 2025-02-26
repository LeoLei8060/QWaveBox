#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QObject>
#include <QString>
#include <QVideoWidget>
#include <QMediaPlayer>
#include <QTimer>

// 使用Qt的媒体播放器类，同时预留FFmpeg的接口
// 在后续版本中将使用FFmpeg实现更复杂的视频处理功能

class VideoPlayer : public QObject {
    Q_OBJECT
    
public:
    enum MediaState {
        StoppedState,
        PlayingState,
        PausedState
    };
    
    enum MediaStatus {
        NoMedia,
        LoadingMedia,
        LoadedMedia,
        InvalidMedia
    };
    
    explicit VideoPlayer(QObject *parent = nullptr);
    ~VideoPlayer();
    
    QVideoWidget* videoWidget() const;
    
    // 媒体控制
    void setMedia(const QString &filePath);
    void play();
    void pause();
    void stop();
    void setPosition(qint64 position);
    void setVolume(int volume);
    void setPlaybackRate(qreal rate);
    
    // 状态查询
    MediaState state() const;
    MediaStatus mediaStatus() const;
    qint64 position() const;
    qint64 duration() const;
    int volume() const;
    qreal playbackRate() const;
    
signals:
    void mediaStateChanged(MediaState state);
    void mediaStatusChanged(MediaStatus status);
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void volumeChanged(int volume);
    void playbackRateChanged(qreal rate);
    
private slots:
    void onPlayerStateChanged(QMediaPlayer::State state);
    void onPlayerMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onPlayerPositionChanged(qint64 position);
    void onPlayerDurationChanged(qint64 duration);
    
private:
    QMediaPlayer *m_mediaPlayer;
    QVideoWidget *m_videoWidget;
    QTimer *m_positionUpdateTimer;
    
    MediaState m_currentState;
    MediaStatus m_currentMediaStatus;
    qint64 m_currentPosition;
    qint64 m_currentDuration;
    int m_currentVolume;
    qreal m_currentPlaybackRate;
    
    // FFmpeg 相关成员变量将在后续版本添加
};

#endif // VIDEOPLAYER_H
