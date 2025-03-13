#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QWidget>

extern "C" {
#include <libavformat/avformat.h>
}

namespace Ui {
class VideoWidget;
}

class SDLWidget;

class VideoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoWidget(QWidget *parent = nullptr);
    ~VideoWidget();

    void renderFrame(AVFrame *frame);

    SDLWidget *getSDLWidget() const;

    void updateProgress(double val);

    void updateTotalDurationStr(int64_t val);
    void updateCurrentDurationStr(const QString &val);

    void updateUIForStateChanged();

signals:
    void sigVolumeChanged(int volume);

    void sigStartPlay();
    void sigPausePlay();
    void sigStopPlay();

    void sigOpenFileDlg();

    void sigSeekTo(int);

private slots:
    void onPreviousBtnClicked();
    void onNextBtnClicked();

private:
    void setupControls();
    void setupVideoWidget();
    void initConnect();

private:
    Ui::VideoWidget *ui;
    int              m_volume{99};
};

#endif // VIDEOWIDGET_H
