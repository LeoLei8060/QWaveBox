#include "mediacontrolwidget.h"

#include <QStyle>
#include <QApplication>

MediaControlWidget::MediaControlWidget(QWidget *parent)
    : QWidget(parent)
    , m_isPlaying(false)
    , m_duration(0)
    , m_position(0)
    , m_isMuted(false)
    , m_lastVolume(50) {
    
    // 创建播放控制按钮
    m_playPauseButton = new QPushButton(this);
    m_playPauseButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
    m_playPauseButton->setToolTip(tr("Play"));
    
    m_stopButton = new QPushButton(this);
    m_stopButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaStop));
    m_stopButton->setToolTip(tr("Stop"));
    
    m_prevButton = new QPushButton(this);
    m_prevButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaSkipBackward));
    m_prevButton->setToolTip(tr("Previous"));
    
    m_nextButton = new QPushButton(this);
    m_nextButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaSkipForward));
    m_nextButton->setToolTip(tr("Next"));
    
    // 创建进度控制
    m_positionSlider = new QSlider(Qt::Horizontal, this);
    m_positionSlider->setRange(0, 0);
    m_positionSlider->setToolTip(tr("Seek"));
    
    m_currentTimeLabel = new QLabel("00:00:00", this);
    m_totalTimeLabel = new QLabel("00:00:00", this);
    
    // 创建音量控制
    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(m_lastVolume);
    m_volumeSlider->setMaximumWidth(100);
    m_volumeSlider->setToolTip(tr("Volume"));
    
    m_muteButton = new QPushButton(this);
    m_muteButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaVolume));
    m_muteButton->setToolTip(tr("Mute"));
    m_muteButton->setCheckable(true);
    
    m_volumeLabel = new QLabel(tr("Volume: %1%").arg(m_volumeSlider->value()), this);
    
    // 创建播放速度控制
    m_speedComboBox = new QComboBox(this);
    m_speedComboBox->addItem("0.5x", 0.5);
    m_speedComboBox->addItem("0.75x", 0.75);
    m_speedComboBox->addItem("1.0x", 1.0);
    m_speedComboBox->addItem("1.25x", 1.25);
    m_speedComboBox->addItem("1.5x", 1.5);
    m_speedComboBox->addItem("1.75x", 1.75);
    m_speedComboBox->addItem("2.0x", 2.0);
    m_speedComboBox->setCurrentIndex(2); // 默认1.0x
    
    m_speedLabel = new QLabel(tr("Speed:"), this);
    
    // 布局
    QHBoxLayout *timeLayout = new QHBoxLayout();
    timeLayout->addWidget(m_currentTimeLabel);
    timeLayout->addWidget(m_positionSlider);
    timeLayout->addWidget(m_totalTimeLabel);
    
    QHBoxLayout *controlLayout = new QHBoxLayout();
    controlLayout->addWidget(m_prevButton);
    controlLayout->addWidget(m_playPauseButton);
    controlLayout->addWidget(m_stopButton);
    controlLayout->addWidget(m_nextButton);
    controlLayout->addStretch();
    controlLayout->addWidget(m_muteButton);
    controlLayout->addWidget(m_volumeSlider);
    controlLayout->addWidget(m_volumeLabel);
    controlLayout->addStretch();
    controlLayout->addWidget(m_speedLabel);
    controlLayout->addWidget(m_speedComboBox);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(timeLayout);
    mainLayout->addLayout(controlLayout);
    
    // 连接信号与槽
    connect(m_playPauseButton, &QPushButton::clicked, this, &MediaControlWidget::onPlayPauseClicked);
    connect(m_stopButton, &QPushButton::clicked, this, &MediaControlWidget::onStopClicked);
    connect(m_prevButton, &QPushButton::clicked, this, &MediaControlWidget::onPreviousClicked);
    connect(m_nextButton, &QPushButton::clicked, this, &MediaControlWidget::onNextClicked);
    
    connect(m_positionSlider, &QSlider::sliderMoved, this, &MediaControlWidget::onPositionSliderMoved);
    connect(m_positionSlider, &QSlider::sliderReleased, this, &MediaControlWidget::onPositionSliderReleased);
    
    connect(m_volumeSlider, &QSlider::valueChanged, this, &MediaControlWidget::onVolumeSliderValueChanged);
    connect(m_muteButton, &QPushButton::toggled, this, [this](bool checked) {
        m_isMuted = checked;
        if (checked) {
            m_lastVolume = m_volumeSlider->value();
            m_volumeSlider->setValue(0);
            m_muteButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaVolumeMuted));
            emit volumeChanged(0);
        } else {
            m_volumeSlider->setValue(m_lastVolume);
            m_muteButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaVolume));
            emit volumeChanged(m_lastVolume);
        }
    });
    
    connect(m_speedComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MediaControlWidget::onSpeedComboBoxIndexChanged);
    
    // 设置初始状态
    updatePlaybackState(false);
}

void MediaControlWidget::updateDuration(qint64 duration) {
    m_duration = duration;
    m_positionSlider->setRange(0, duration);
    m_totalTimeLabel->setText(formatTime(duration));
}

void MediaControlWidget::updatePosition(qint64 position) {
    if (!m_positionSlider->isSliderDown()) {
        m_position = position;
        m_positionSlider->setValue(position);
        m_currentTimeLabel->setText(formatTime(position));
    }
}

void MediaControlWidget::updatePlaybackState(bool isPlaying) {
    m_isPlaying = isPlaying;
    if (isPlaying) {
        m_playPauseButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPause));
        m_playPauseButton->setToolTip(tr("Pause"));
    } else {
        m_playPauseButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
        m_playPauseButton->setToolTip(tr("Play"));
    }
}

void MediaControlWidget::onPlayPauseClicked() {
    if (m_isPlaying) {
        emit pause();
    } else {
        emit play();
    }
}

void MediaControlWidget::onStopClicked() {
    emit stop();
}

void MediaControlWidget::onPreviousClicked() {
    emit previous();
}

void MediaControlWidget::onNextClicked() {
    emit next();
}

void MediaControlWidget::onPositionSliderMoved(int position) {
    m_currentTimeLabel->setText(formatTime(position));
}

void MediaControlWidget::onPositionSliderReleased() {
    emit positionChanged(m_positionSlider->value());
}

void MediaControlWidget::onVolumeSliderValueChanged(int value) {
    m_volumeLabel->setText(tr("Volume: %1%").arg(value));
    
    if (value == 0) {
        if (!m_isMuted) {
            m_muteButton->setChecked(true);
        }
    } else if (m_isMuted) {
        m_muteButton->setChecked(false);
    }
    
    emit volumeChanged(value);
}

void MediaControlWidget::onSpeedComboBoxIndexChanged(int index) {
    if (index >= 0 && index < m_speedRates.size()) {
        emit speedChanged(m_speedRates[index]);
    }
}

QString MediaControlWidget::formatTime(qint64 milliseconds) {
    QTime time(0, 0);
    time = time.addMSecs(milliseconds);
    if (m_duration > 3600000) { // 超过1小时
        return time.toString("hh:mm:ss");
    } else {
        return time.toString("mm:ss");
    }
}
