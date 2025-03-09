#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include "threadmanager.h"

#include <QAction>
#include <QMenu>
#include <QShowEvent>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QWidget>

namespace Ui {
class MainWidget;
}

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    // 窗口边缘类型枚举
    enum class EdgeType {
        None = 0,
        Left = 1,
        Top = 2,
        Right = 3,
        Bottom = 4,
        TopLeft = 5,
        TopRight = 6,
        BottomLeft = 7,
        BottomRight = 8
    };

    explicit MainWidget(QWidget *parent = nullptr);
    ~MainWidget();

protected:
    void changeEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    // 处理TitleBar发出的信号的槽函数
    void onOpenFileDlg();
    void onOpenFolder();
    void onCloseToTray();
    void onOptions();
    void onAbout();
    void onQuitApplication();

    // 托盘图标槽函数
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);

    // 打开视频文件
    void onOpenFile(const QString &filePath);

    // 更新播放状态
    void onPlayStateChanged(PlayState state);

private:
    // 初始化函数
    void connectTitleBarSignals();
    void setupPlayListWidget();
    void setupVideoWidget();
    void setupMenu();
    void setupTrayIcon();
    void setupThreadManager();

private:
    Ui::MainWidget *ui;
    bool            m_isMaximized = false;
    bool            m_resizing = false;
    QPoint          m_dragPos;
    EdgeType        m_dragEdge = EdgeType::None;
    int             m_borderWidth = 2; // Width of the resize area

    // 菜单和托盘相关
    QMenu           *m_menu;
    QSystemTrayIcon *m_trayIcon;
    QMenu           *m_trayMenu;

    std::unique_ptr<ThreadManager> m_threadManager;

    QTimer  m_timer;
    int64_t m_testTime{0};
};

#endif // MAINWIDGET_H
