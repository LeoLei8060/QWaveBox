#ifndef MEDIACONTROLWIDGET_H
#define MEDIACONTROLWIDGET_H

#include <QWidget>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTime>

class MediaControlWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit MediaControlWidget(QWidget *parent = nullptr);
    
    void updateDuration(qint64 duration);
    void updatePosition(qint64 position);
    void updatePlaybackState(bool isPlaying);
    
signals:
    void play();
    void pause();
    void stop();
    void previous();
    void next();
    void positionChanged(qint64 position);
    void volumeChanged(int volume);
    void speedChanged(qreal speed);
    
private slots:
    void onPlayPauseClicked();
    void onStopClicked();
    void onPreviousClicked();
    void onNextClicked();
    void onPositionSliderMoved(int position);
    void onPositionSliderReleased();
    void onVolumeSliderValueChanged(int value);
    void onSpeedComboBoxIndexChanged(int index);
    
private:
    QString formatTime(qint64 milliseconds);
    
    // 控制按钮
    QPushButton *m_playPauseButton;
    QPushButton *m_stopButton;
    QPushButton *m_prevButton;
    QPushButton *m_nextButton;
    
    // 进度控制
    QSlider *m_positionSlider;
    QLabel *m_currentTimeLabel;
    QLabel *m_totalTimeLabel;
    
    // 音量控制
    QSlider *m_volumeSlider;
    QPushButton *m_muteButton;
    QLabel *m_volumeLabel;
    
    // 播放速度控制
    QComboBox *m_speedComboBox;
    QLabel *m_speedLabel;
    
    // 状态
    bool m_isPlaying;
    qint64 m_duration;
    qint64 m_position;
    bool m_isMuted;
    int m_lastVolume;
    
    // 播放速度选项
    const QList<qreal> m_speedRates = {0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0};
};

#endif // MEDIACONTROLWIDGET_H
