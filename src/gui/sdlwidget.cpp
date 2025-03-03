#include "sdlwidget.h"

#include <SDL_render.h>
#include <QDebug>
#include <QPainter>

SDLWidget::SDLWidget(QWidget *parent)
    : QWidget{parent}
{
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

SDLWidget::~SDLWidget()
{
    // 释放SDL资源
    if (m_texture) {
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
    }

    if (m_sdlRenderer) {
        SDL_DestroyRenderer(m_sdlRenderer);
        m_sdlRenderer = nullptr;
    }

    if (m_sdlWindow) {
        SDL_DestroyWindow(m_sdlWindow);
        m_sdlWindow = nullptr;
    }
}

bool SDLWidget::initializeSDL()
{
    WId wid = winId();
    if (wid == 0)
        return false;

    SDL_Init(SDL_INIT_VIDEO);

    // 绑定窗口
    m_sdlWindow = SDL_CreateWindowFrom((void *) wid);
    if (!m_sdlWindow) {
        qWarning() << "SDL 窗口创建失败: " << SDL_GetError();
        return false;
    }

    // 创建渲染器
    m_sdlRenderer = SDL_CreateRenderer(m_sdlWindow,
                                       -1,
                                       SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC);
    if (!m_sdlRenderer) {
        qWarning() << "SDL 渲染器创建失败: " << SDL_GetError();
        return false;
    }

    return true;
}

void SDLWidget::renderFrame(AVFrame *frame)
{
    if (!frame || !m_sdlRenderer)
        return;

    // 如果尺寸变化或还未创建纹理，则创建新的纹理
    if (!m_texture || frame->width != m_textureWidth || frame->height != m_textureHeight) {
        if (m_texture) {
            SDL_DestroyTexture(m_texture);
            m_texture = nullptr;
        }

        // 为YUV格式创建纹理
        m_textureWidth = frame->width;
        m_textureHeight = frame->height;

        // 对应于YUV420P格式
        m_texture = SDL_CreateTexture(m_sdlRenderer,
                                      SDL_PIXELFORMAT_IYUV, // 对应YUV420P格式
                                      SDL_TEXTUREACCESS_STREAMING,
                                      frame->width,
                                      frame->height);

        if (!m_texture) {
            qWarning() << "SDL 纹理创建失败: " << SDL_GetError();
            return;
        }
    }

    // 更新YUV数据到纹理
    // YUV420P格式下，Y平面占用整个图像的像素，U和V平面各占1/4
    SDL_UpdateYUVTexture(m_texture,
                         NULL,
                         frame->data[0],
                         frame->linesize[0], // Y平面
                         frame->data[1],
                         frame->linesize[1], // U平面
                         frame->data[2],
                         frame->linesize[2] // V平面
    );

    // 清除渲染器
    SDL_RenderClear(m_sdlRenderer);

    // 计算保持宽高比的目标矩形
    SDL_Rect srcRect = {0, 0, frame->width, frame->height};
    SDL_Rect dstRect = {0, 0, width(), height()};

    // 计算按比例缩放的矩形
    float srcAspectRatio = static_cast<float>(frame->width) / frame->height;
    float dstAspectRatio = static_cast<float>(width()) / height();

    if (srcAspectRatio > dstAspectRatio) {
        // 视频比窗口更宽，以宽度为准，调整高度
        dstRect.h = static_cast<int>(width() / srcAspectRatio);
        dstRect.y = (height() - dstRect.h) / 2;
    } else {
        // 视频比窗口更高，以高度为准，调整宽度
        dstRect.w = static_cast<int>(height() * srcAspectRatio);
        dstRect.x = (width() - dstRect.w) / 2;
    }

    // 将纹理复制到渲染器
    SDL_RenderCopy(m_sdlRenderer, m_texture, &srcRect, &dstRect);

    // 更新屏幕
    SDL_RenderPresent(m_sdlRenderer);
}

void SDLWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    // 如果没有SDL渲染器，则使用Qt的绘制方法绘制黑色背景
    if (!m_sdlRenderer) {
        QPainter painter(this);
        painter.fillRect(rect(), Qt::black);
    }
    // 如果有SDL渲染器，则不需要做额外处理，因为SDL已经处理了渲染
}

void SDLWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);

    // 当窗口大小改变时，不需要重新创建SDL窗口或渲染器
    // 因为SDL_Window是从QWidget的窗口句柄创建的，会自动处理大小变化
    // 在下一次renderFrame时会根据新尺寸调整显示矩形
}

void SDLWidget::reset()
{
    // 释放纹理资源
    if (m_texture) {
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
    }

    // 重置纹理尺寸
    m_textureWidth = 0;
    m_textureHeight = 0;

    // 重新绘制窗口(黑色背景)
    update();
}
