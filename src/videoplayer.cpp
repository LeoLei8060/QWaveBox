#include "videoplayer.h"

#include <QMediaContent>
#include <QUrl>
#include <QFileInfo>

VideoPlayer::VideoPlayer(QObject *parent)
    : QObject(parent)
    , m_mediaPlayer(new QMediaPlayer(this))
    , m_videoWidget(new QVideoWidget())
    , m_positionUpdateTimer(new QTimer(this))
    , m_currentState(StoppedState)
    , m_currentMediaStatus(NoMedia)
    , m_currentPosition(0)
    , m_currentDuration(0)
    , m_currentVolume(50)
    , m_currentPlaybackRate(1.0) {
    
    // 设置视频输出
    m_mediaPlayer->setVideoOutput(m_videoWidget);
    
    // 连接信号槽
    connect(m_mediaPlayer, &QMediaPlayer::stateChanged, this, &VideoPlayer::onPlayerStateChanged);
    connect(m_mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &VideoPlayer::onPlayerMediaStatusChanged);
    connect(m_mediaPlayer, &QMediaPlayer::positionChanged, this, &VideoPlayer::onPlayerPositionChanged);
    connect(m_mediaPlayer, &QMediaPlayer::durationChanged, this, &VideoPlayer::onPlayerDurationChanged);
    
    // 设置位置更新定时器，提供更平滑的位置更新
    m_positionUpdateTimer->setInterval(200); // 200ms
    connect(m_positionUpdateTimer, &QTimer::timeout, this, [this]() {
        emit positionChanged(m_mediaPlayer->position());
    });
    
    // 设置默认音量
    m_mediaPlayer->setVolume(m_currentVolume);
}

VideoPlayer::~VideoPlayer() {
    delete m_videoWidget;
}

QVideoWidget* VideoPlayer::videoWidget() const {
    return m_videoWidget;
}

void VideoPlayer::setMedia(const QString &filePath) {
    if (filePath.isEmpty()) {
        return;
    }
    
    // 停止当前播放
    stop();
    
    // 设置新的媒体
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists()) {
        QUrl url = QUrl::fromLocalFile(filePath);
        m_mediaPlayer->setMedia(QMediaContent(url));
        emit mediaStatusChanged(LoadingMedia);
    } else {
        // 检查是否是网络URL
        QUrl url(filePath);
        if (url.isValid() && (url.scheme() == "http" || url.scheme() == "https")) {
            m_mediaPlayer->setMedia(QMediaContent(url));
            emit mediaStatusChanged(LoadingMedia);
        } else {
            emit mediaStatusChanged(InvalidMedia);
        }
    }
}

void VideoPlayer::play() {
    if (m_mediaPlayer->mediaStatus() != QMediaPlayer::NoMedia) {
        m_mediaPlayer->play();
        m_positionUpdateTimer->start();
    }
}

void VideoPlayer::pause() {
    m_mediaPlayer->pause();
    m_positionUpdateTimer->stop();
}

void VideoPlayer::stop() {
    m_mediaPlayer->stop();
    m_positionUpdateTimer->stop();
}

void VideoPlayer::setPosition(qint64 position) {
    m_mediaPlayer->setPosition(position);
}

void VideoPlayer::setVolume(int volume) {
    m_currentVolume = qBound(0, volume, 100);
    m_mediaPlayer->setVolume(m_currentVolume);
    emit volumeChanged(m_currentVolume);
}

void VideoPlayer::setPlaybackRate(qreal rate) {
    m_currentPlaybackRate = qBound(0.25, rate, 4.0);
    m_mediaPlayer->setPlaybackRate(m_currentPlaybackRate);
    emit playbackRateChanged(m_currentPlaybackRate);
}

VideoPlayer::MediaState VideoPlayer::state() const {
    return m_currentState;
}

VideoPlayer::MediaStatus VideoPlayer::mediaStatus() const {
    return m_currentMediaStatus;
}

qint64 VideoPlayer::position() const {
    return m_mediaPlayer->position();
}

qint64 VideoPlayer::duration() const {
    return m_mediaPlayer->duration();
}

int VideoPlayer::volume() const {
    return m_currentVolume;
}

qreal VideoPlayer::playbackRate() const {
    return m_currentPlaybackRate;
}

void VideoPlayer::onPlayerStateChanged(QMediaPlayer::State state) {
    switch (state) {
        case QMediaPlayer::StoppedState:
            m_currentState = StoppedState;
            break;
        case QMediaPlayer::PlayingState:
            m_currentState = PlayingState;
            break;
        case QMediaPlayer::PausedState:
            m_currentState = PausedState;
            break;
    }
    
    emit mediaStateChanged(m_currentState);
}

void VideoPlayer::onPlayerMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    switch (status) {
        case QMediaPlayer::NoMedia:
            m_currentMediaStatus = NoMedia;
            break;
        case QMediaPlayer::LoadingMedia:
            m_currentMediaStatus = LoadingMedia;
            break;
        case QMediaPlayer::LoadedMedia:
        case QMediaPlayer::BufferedMedia:
            m_currentMediaStatus = LoadedMedia;
            break;
        case QMediaPlayer::InvalidMedia:
            m_currentMediaStatus = InvalidMedia;
            break;
        default:
            // 其他状态暂不处理
            break;
    }
    
    emit mediaStatusChanged(m_currentMediaStatus);
}

void VideoPlayer::onPlayerPositionChanged(qint64 position) {
    m_currentPosition = position;
    // 不直接发送信号，通过定时器更新，减少UI更新频率
}

void VideoPlayer::onPlayerDurationChanged(qint64 duration) {
    m_currentDuration = duration;
    emit durationChanged(m_currentDuration);
}
