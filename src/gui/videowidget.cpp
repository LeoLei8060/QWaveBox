#include "videowidget.h"
#include "fontmanager.h"
#include "ui_videowidget.h"

VideoWidget::VideoWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::VideoWidget)
{
    ui->setupUi(this);

    setupControls();
    setupVideoWidget();
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

void VideoWidget::setupControls()
{
    auto font = FontManager::instance()->fontAt(FontManager::IconFont);

    ui->playBtn->setFont(font);
    ui->playBtn->setText(QChar(0xe614));

    ui->stopBtn->setFont(font);
    ui->stopBtn->setText(QChar(0xe91d));

    ui->previousBtn->setFont(font);
    ui->previousBtn->setText(QChar(0xe616));

    ui->nextBtn->setFont(font);
    ui->nextBtn->setText(QChar(0xe617));

    ui->openBtn->setFont(font);
    ui->openBtn->setText(QChar(0xe6d3));

    ui->playlistBtn->setFont(font);
    ui->playlistBtn->setText(QChar(0xe602));

    ui->voiceBtn->setFont(font);
    ui->voiceBtn->setText(QChar(0xea11));
}

void VideoWidget::setupVideoWidget()
{
    ui->video->initializeSDL();
}
