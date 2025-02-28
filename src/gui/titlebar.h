#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <QWidget>
#include <QMouseEvent>
#include <QFont>

namespace Ui {
class TitleBar;
}

class TitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(QWidget *parent = nullptr);
    ~TitleBar();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onPinButtonClicked();
    void onMinimizeButtonClicked();
    void onMaximizeButtonClicked();
    void onFullscreenButtonClicked();
    void onCloseButtonClicked();

private:
    void setupButtons();
    void updateButtonStates();

private:
    Ui::TitleBar *ui;
    QPoint m_dragPosition;
    bool m_isPressed = false;
    bool m_isPinned = false;
    bool m_isMaximized = false;
    bool m_isFullscreen = false;
    QFont m_iconFont;
};

#endif // TITLEBAR_H
