#include "videowidget.h"
#include "appcontext.h"
#include "fontmanager.h"
#include "ui_videowidget.h"
#include <QDebug>

#define PLAY_BTN_TEXT     QChar(0xe614)
#define STOP_BTN_TEXT     QChar(0xe91d)
#define PAUSE_BTN_TEXT    QChar(0xe610)
#define PREV_BTN_TEXT     QChar(0xe616)
#define NEXT_BTN_TEXT     QChar(0xe617)
#define PLAYLIST_BTN_TEXT QChar(0xe602)
#define VOICE_BTN_TEXT    QChar(0xea11)
#define MUTE_BTN_TEXT     QChar(0xe611)
#define OPEN_BTN_TEXT     QChar(0xe6d3)

VideoWidget::VideoWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::VideoWidget)
{
    ui->setupUi(this);

    setupControls();
    setupVideoWidget();
    initConnect();
}

VideoWidget::~VideoWidget()
{
    delete ui;
}

void VideoWidget::renderFrame(AVFrame *frame)
{
    ui->video->renderFrame(frame);
}

SDLWidget *VideoWidget::getSDLWidget() const
{
    return ui->video;
}

void VideoWidget::updateProgress(double val)
{
    // 进度条的范围[0, 99999]
    // val值是百分比，需要乘以100000
    int iVal = val * 100000;
    ui->videoSlider->setValue(iVal);
}

void VideoWidget::updateTotalDurationStr(const QString &val)
{
    ui->label_totalduration->setText(val);
}

void VideoWidget::updateCurrentDurationStr(const QString &val)
{
    ui->label_currentduration->setText(val);
}

void VideoWidget::updateUIForStateChanged()
{
    auto state = AppContext::instance()->getPlayState();
    if (state == PlayState::PlayingState) {
        // 正在播放状态
        ui->playBtn->setText(PAUSE_BTN_TEXT);
    } else if (state == PlayState::PausedState) {
        // 暂停状态
        ui->playBtn->setText(PLAY_BTN_TEXT);
    } else {
        // 停止状态
        ui->playBtn->setText(PLAY_BTN_TEXT);
    }
}

void VideoWidget::setupControls()
{
    auto font = FontManager::instance()->fontAt(FontManager::IconFont);

    ui->stopBtn->setFont(font);
    ui->stopBtn->setText(STOP_BTN_TEXT);

    ui->playlistBtn->setFont(font);
    ui->playlistBtn->setText(PLAYLIST_BTN_TEXT);

    font.setPixelSize(16);
    ui->playBtn->setFont(font);
    ui->playBtn->setText(PLAY_BTN_TEXT);

    ui->previousBtn->setFont(font);
    ui->previousBtn->setText(PREV_BTN_TEXT);

    ui->nextBtn->setFont(font);
    ui->nextBtn->setText(NEXT_BTN_TEXT);

    ui->openBtn->setFont(font);
    ui->openBtn->setText(OPEN_BTN_TEXT);

    ui->voiceBtn->setFont(font);
    ui->voiceBtn->setText(VOICE_BTN_TEXT);
}

void VideoWidget::setupVideoWidget()
{
    ui->video->initializeSDL();
}

void VideoWidget::initConnect()
{
    connect(ui->voiceSlider, &QSlider::valueChanged, this, &VideoWidget::sigVolumeChanged);
    connect(ui->playBtn, &QPushButton::clicked, this, &VideoWidget::sigStartPlay);
    connect(ui->stopBtn, &QPushButton::clicked, this, &VideoWidget::sigStopPlay);
}
