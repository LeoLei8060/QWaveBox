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

private:
    void setupControls();
    void setupVideoWidget();

private:
    Ui::VideoWidget *ui;
};

#endif // VIDEOWIDGET_H
