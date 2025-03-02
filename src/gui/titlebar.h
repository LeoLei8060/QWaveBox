#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <QWidget>
#include <QMouseEvent>
#include <QFont>
#include <QMenu>
#include <QAction>
#include <QShortcut>

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

signals:
    void openFileRequested();
    void openFolderRequested();
    void closeToTrayRequested();
    void optionsRequested();
    void aboutRequested();
    void quitApplicationRequested();

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
    void setupGlobalShortcuts();

private:
    Ui::TitleBar *ui;
    QPoint m_dragPosition;
    bool m_isPressed = false;
    bool m_isPinned = false;
    bool m_isMaximized = false;
    bool m_isFullscreen = false;
    QFont m_iconFont;
    
    // 只保留快捷键，菜单将由主窗口提供
    QShortcut *m_openFileShortcut;
    QShortcut *m_openFolderShortcut;
    QShortcut *m_closeToTrayShortcut;
    QShortcut *m_optionsShortcut;
    QShortcut *m_aboutShortcut;
    QShortcut *m_quitShortcut;
};

#endif // TITLEBAR_H
