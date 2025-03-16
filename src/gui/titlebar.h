#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <QAction>
#include <QFont>
#include <QMenu>
#include <QMouseEvent>
#include <QShortcut>
#include <QWidget>

namespace Ui {
class TitleBar;
}

class TitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(QWidget *parent = nullptr);
    ~TitleBar();

    // 提供设置菜单的公共方法
    void setMenu(QMenu *menu);

public slots:
    void onFullscreenButtonClicked();

signals:
    void closeToTrayRequested();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onPinButtonClicked();
    void onMinimizeButtonClicked();
    void onMaximizeButtonClicked();
    void onCloseButtonClicked();

private:
    void setupButtons();
    void updateButtonStates();
    // void setupGlobalShortcuts();

private:
    Ui::TitleBar *ui;
    QPoint        m_dragPosition;
    bool          m_isPressed = false;
    bool          m_isPinned = false;
    bool          m_isMaximized = false;
    bool          m_isFullscreen = false;
    QFont         m_iconFont;
};

#endif // TITLEBAR_H
