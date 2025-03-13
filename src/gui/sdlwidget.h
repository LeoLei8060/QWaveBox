#ifndef SDLWIDGET_H
#define SDLWIDGET_H

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <QImage>
#include <QWidget>

extern "C" {
#include <libavformat/avformat.h>
}

class SDLWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SDLWidget(QWidget *parent = nullptr);
    ~SDLWidget();

    bool initializeSDL();

    void renderFrame(AVFrame *frame);

    // 重置渲染器状态
    void reset();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    SDL_Window   *m_sdlWindow{nullptr};
    SDL_Renderer *m_sdlRenderer{nullptr};
    SDL_Texture  *m_texture{nullptr};
    int           m_textureWidth{0};  // 当前纹理宽度
    int           m_textureHeight{0}; // 当前纹理高度

    QImage m_backimg;
};

#endif // SDLWIDGET_H
