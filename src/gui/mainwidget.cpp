#include "MainWidget.h"
#include "appcontext.h"
#include "audiodecodethread.h"
#include "audiorenderthread.h"
#include "common.h"
#include "demuxthread.h"
#include "renderthread.h"
#include "sdlwidget.h"
#include "shortcutmanager.h"
#include "titlebar.h"
#include "ui_mainwidget.h"
#include "videodecodethread.h"

#include <QApplication>
#include <QCloseEvent>
#include <QCursor>
#include <QDebug>
#include <QEvent>
#include <QFileDialog>
#include <QIcon>
#include <QMessageBox>
#include <QMouseEvent>
#include <QScreen>
#include <QStandardPaths>
#include <QStyle>
#include <QWindow>

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWidget)
    , m_threadManager(nullptr)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setMouseTracking(true);

    // 设置应用程序图标
    QIcon appIcon(":/resources/wavebox.ico");
    setWindowIcon(appIcon);

    ui->playlistWidget->setCursor(Qt::ArrowCursor);
    ui->videoWidget->setCursor(Qt::ArrowCursor);

    setupHotkeys();
    setupPlayListWidget();
    setupVideoWidget();
    // 初始化菜单和托盘
    setupMenu();
    setupTrayIcon();

    // 刷新UI的定时器
    m_refreshTimer.setInterval(200);
    connect(&m_refreshTimer, &QTimer::timeout, this, &MainWidget::onTimedRefreshUI);

    resize(800, 600);

    // Center window on screen
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        const QRect availableGeometry = screen->availableGeometry();
        setGeometry(
            QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), availableGeometry));
    }

    // 初始化线程管理器
    setupThreadManager();
}

MainWidget::~MainWidget()
{
    delete ui;
    delete m_menu;
    delete m_trayMenu;
    delete m_trayIcon;
}

void MainWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
        if (windowState() & Qt::WindowMaximized) {
            m_isMaximized = true;
        } else if (windowState() & Qt::WindowNoState) {
            m_isMaximized = false;
        }
    }

    QWidget::changeEvent(event);
}

void MainWidget::mousePressEvent(QMouseEvent *event)
{
    if (m_isMaximized) {
        QWidget::mousePressEvent(event);
        return;
    }

    m_dragPos = event->globalPos();

    // Check 鼠标位置是否在边框上
    const QRect frameGeometry = this->frameGeometry();
    const int   x = event->x();
    const int   y = event->y();
    const int   width = frameGeometry.width();
    const int   height = frameGeometry.height();

    if (x <= m_borderWidth && y <= m_borderWidth) {
        m_dragEdge = MainWidget::EdgeType::TopLeft; // Top-left corner
    } else if (x <= m_borderWidth && y >= height - m_borderWidth) {
        m_dragEdge = MainWidget::EdgeType::BottomLeft; // Bottom-left corner
    } else if (x >= width - m_borderWidth && y <= m_borderWidth) {
        m_dragEdge = MainWidget::EdgeType::TopRight; // Top-right corner
    } else if (x >= width - m_borderWidth && y >= height - m_borderWidth) {
        m_dragEdge = MainWidget::EdgeType::BottomRight; // Bottom-right corner
    } else if (x <= m_borderWidth) {
        m_dragEdge = MainWidget::EdgeType::Left; // Left edge
    } else if (y <= m_borderWidth) {
        m_dragEdge = MainWidget::EdgeType::Top; // Top edge
    } else if (x >= width - m_borderWidth) {
        m_dragEdge = MainWidget::EdgeType::Right; // Right edge
    } else if (y >= height - m_borderWidth) {
        m_dragEdge = MainWidget::EdgeType::Bottom; // Bottom edge
    } else {
        m_dragEdge = MainWidget::EdgeType::None; // No edge
    }

    if (m_dragEdge != MainWidget::EdgeType::None) {
        m_resizing = true;
    }

    QWidget::mousePressEvent(event);
}

void MainWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isMaximized) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    // Resize 状态
    if (m_resizing) {
        const QPoint globalPos = event->globalPos();
        QRect        newGeometry = geometry();

        // Calculate the delta
        const int dx = globalPos.x() - m_dragPos.x();
        const int dy = globalPos.y() - m_dragPos.y();

        // Update drag position
        m_dragPos = globalPos;

        switch (m_dragEdge) {
        case MainWidget::EdgeType::Left: // Left edge
            newGeometry.setLeft(newGeometry.left() + dx);
            break;
        case MainWidget::EdgeType::Top: // Top edge
            newGeometry.setTop(newGeometry.top() + dy);
            break;
        case MainWidget::EdgeType::Right: // Right edge
            newGeometry.setRight(newGeometry.right() + dx);
            break;
        case MainWidget::EdgeType::Bottom: // Bottom edge
            newGeometry.setBottom(newGeometry.bottom() + dy);
            break;
        case MainWidget::EdgeType::TopLeft: // Top-left corner
            newGeometry.setTopLeft(newGeometry.topLeft() + QPoint(dx, dy));
            break;
        case MainWidget::EdgeType::TopRight: // Top-right corner
            newGeometry.setTopRight(newGeometry.topRight() + QPoint(dx, dy));
            break;
        case MainWidget::EdgeType::BottomLeft: // Bottom-left corner
            newGeometry.setBottomLeft(newGeometry.bottomLeft() + QPoint(dx, dy));
            break;
        case MainWidget::EdgeType::BottomRight: // Bottom-right corner
            newGeometry.setBottomRight(newGeometry.bottomRight() + QPoint(dx, dy));
            break;
        case EdgeType::None:
            break;
        }

        setGeometry(newGeometry);
    } else {
        // 更新鼠标样式
        const QRect frameGeometry = this->frameGeometry();
        const int   x = event->x();
        const int   y = event->y();
        const int   width = frameGeometry.width();
        const int   height = frameGeometry.height();

        if ((x <= m_borderWidth && y <= m_borderWidth)
            || (x >= width - m_borderWidth && y >= height - m_borderWidth)) {
            setCursor(Qt::SizeFDiagCursor);
        } else if ((x <= m_borderWidth && y >= height - m_borderWidth)
                   || (x >= width - m_borderWidth && y <= m_borderWidth)) {
            setCursor(Qt::SizeBDiagCursor);
        } else if (x <= m_borderWidth || x >= width - m_borderWidth) {
            setCursor(Qt::SizeHorCursor);
        } else if (y <= m_borderWidth || y >= height - m_borderWidth) {
            setCursor(Qt::SizeVerCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
    }

    QWidget::mouseMoveEvent(event);
}

void MainWidget::mouseReleaseEvent(QMouseEvent *event)
{
    m_resizing = false;
    m_dragEdge = MainWidget::EdgeType::None;

    QWidget::mouseReleaseEvent(event);
}

void MainWidget::closeEvent(QCloseEvent *event)
{
    onQuitApplication();
    event->ignore();
}

void MainWidget::setupMenu()
{
    // 创建主菜单
    m_menu = new QMenu(this);

    // 创建菜单项
    QAction *openFileAction = new QAction(tr("打开文件(F3)"), this);
    openFileAction->setShortcut(QKeySequence(Qt::Key_F3));
    QAction *openFolderAction = new QAction(tr("打开文件夹(F2)"), this);
    openFolderAction->setShortcut(QKeySequence("F2"));
    QAction *closeAction = new QAction(tr("关闭(F4)"), this);
    closeAction->setShortcut(QKeySequence("F4"));
    QAction *optionsAction = new QAction(tr("选项(F5)"), this);
    optionsAction->setShortcut(QKeySequence("F5"));
    QAction *aboutAction = new QAction(tr("关于(F12)"), this);
    aboutAction->setShortcut(QKeySequence("F12"));
    QAction *quitAction = new QAction(tr("退出(Ctrl+F4)"), this);
    quitAction->setShortcut(QKeySequence("Ctrl+F4"));

    // 添加菜单项到菜单
    m_menu->addAction(openFileAction);
    m_menu->addAction(openFolderAction);
    m_menu->addAction(closeAction);
    m_menu->addSeparator();
    m_menu->addAction(optionsAction);
    m_menu->addAction(aboutAction);
    m_menu->addSeparator();
    m_menu->addAction(quitAction);

    // 连接信号和槽
    connect(openFileAction, &QAction::triggered, this, &MainWidget::onOpenFileDlg);
    connect(openFolderAction, &QAction::triggered, this, &MainWidget::onOpenFolder);
    connect(closeAction, &QAction::triggered, this, &MainWidget::onCloseToTray);
    connect(optionsAction, &QAction::triggered, this, &MainWidget::onOptions);
    connect(aboutAction, &QAction::triggered, this, &MainWidget::onAbout);
    connect(quitAction, &QAction::triggered, this, &MainWidget::onQuitApplication);

    // 将菜单设置到TitleBar的QToolButton
    if (ui->titlebar) {
        TitleBar *titleBar = qobject_cast<TitleBar *>(ui->titlebar);
        if (titleBar) {
            titleBar->setMenu(m_menu);
        }
    }
}

void MainWidget::setupTrayIcon()
{
    // 创建托盘图标
    m_trayIcon = new QSystemTrayIcon(this);

    // 使用应用程序图标作为托盘图标
    QIcon trayIcon(":/resources/wavebox.ico");
    m_trayIcon->setIcon(trayIcon);
    m_trayIcon->setToolTip(tr("QWaveBox"));

    // 创建托盘菜单
    m_trayMenu = new QMenu(this);

    // 创建托盘菜单项
    QAction *showAction = new QAction(tr("显示"), this);
    QAction *quitAction = new QAction(tr("退出"), this);

    // 添加菜单项到托盘菜单
    m_trayMenu->addAction(showAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(quitAction);

    // 设置托盘菜单
    m_trayIcon->setContextMenu(m_trayMenu);

    // 连接信号和槽
    connect(showAction, &QAction::triggered, this, [=]() {
        show();
        activateWindow();
    });

    connect(quitAction, &QAction::triggered, this, &MainWidget::onQuitApplication);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWidget::onTrayIconActivated);

    // 显示托盘图标
    m_trayIcon->show();
}

void MainWidget::setupThreadManager()
{
    m_threadManager = std::make_unique<ThreadManager>();
    if (!m_threadManager->initializeThreads()) {
        qWarning() << "线程管理器初始化失败...";
        return;
    }
    m_threadManager->setVideoRenderObj(ui->videoWidget->getSDLWidget());

    connect(m_threadManager.get(),
            &ThreadManager::sigPlayStateChanged,
            this,
            &MainWidget::onPlayStateChanged);
    connect(m_threadManager.get(),
            &ThreadManager::sigVoiceStateChanged,
            this,
            &MainWidget::onVoiceStateChanged);
}

void MainWidget::setupPlayListWidget()
{
    connect(ui->playlistWidget, &PlaylistWidget::sigOpenFile, this, &MainWidget::onOpenFile);
}

void MainWidget::setupVideoWidget()
{
    ui->videoWidget->setVolume(AppContext::instance()->getAppData()->getVolume());

    connect(ui->videoWidget, &VideoWidget::sigVolumeChanged, this, [this](int volume) {
        m_threadManager->setVolume(volume);
        AppContext::instance()->setVolume(volume);
    });
    connect(ui->videoWidget, &VideoWidget::sigStopPlay, this, [this]() {
        m_threadManager->stopPlay();
    });
    connect(ui->videoWidget, &VideoWidget::sigStartPlay, this, &MainWidget::onPlayTriggered);
    connect(ui->videoWidget, &VideoWidget::sigSeekTo, this, &MainWidget::onSeekTo);
    connect(ui->videoWidget, &VideoWidget::sigOpenFileDlg, this, &MainWidget::onOpenFileDlg);
    connect(ui->videoWidget, &VideoWidget::sigPlayListStateChanged, this, [this]() {
        if (ui->playlistWidget->isHidden())
            ui->playlistWidget->show();
        else
            ui->playlistWidget->hide();
    });
}

void MainWidget::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        show();
        activateWindow();
    }
}

bool MainWidget::onOpenFile(const QString &filePath)
{
    if (AppContext::instance()->isPlaying() || AppContext::instance()->isPauseed())
        m_threadManager->stopPlay();
    if (!filePath.isEmpty()) {
        qDebug() << "Opening file:" << filePath;
        m_threadManager->openMedia(filePath);

        if (!m_threadManager->startAllThreads()) {
            qWarning() << "线程启动失败...";
            return false;
        }

        auto ms = m_threadManager->getDemuxThread()->getDuration();
        ui->videoWidget->updateTotalDurationStr(ms);
        ui->playlistWidget->addFileToDefaultList(filePath);
        return true;
    }
    return false;
}

void MainWidget::onPlayStateChanged(PlayState state)
{
    AppContext::instance()->setPlayState(state);
    ui->videoWidget->updateUIForStateChanged();
    if (state == PlayState::PlayingState)
        m_refreshTimer.start();
    else
        m_refreshTimer.stop();
}

void MainWidget::onVoiceStateChanged(VoiceState state)
{
    AppContext::instance()->setVoiceState(state);
    ui->videoWidget->updateUIForStateChanged();
}

void MainWidget::onSeekTo(int position)
{
    // position = 当前视频位置（ms）
    if (m_threadManager->isPlaying())
        m_threadManager->seekToPosition(position);
}

void MainWidget::onTimedRefreshUI()
{
    // 进度条更新
    double progress = m_threadManager->getCurrentPlayProgress();
    ui->videoWidget->updateProgress(progress);

    // 时间更新
    QString str = millisecondToString(m_threadManager->getPlayDuration());
    ui->videoWidget->updateCurrentDurationStr(str);
}

void MainWidget::setupHotkeys()
{
    m_key_playSelected = new QShortcut(QKeySequence(Qt::Key_Return), this);
    connect(m_key_playSelected, &QShortcut::activated, this, [this]() {
        ui->playlistWidget->playSelected();
    });

    m_key_pausePlay = new QShortcut(Qt::Key_Space, this);
    connect(m_key_pausePlay, &QShortcut::activated, this, &MainWidget::onPlayTriggered);

    m_key_nextPlay = new QShortcut(Qt::Key_Right, this);
    connect(m_key_nextPlay, &QShortcut::activated, this, [this]() {
        ui->videoWidget->onNextBtnClicked();
    });

    m_key_prevPlay = new QShortcut(Qt::Key_Left, this);
    connect(m_key_prevPlay, &QShortcut::activated, this, [this]() {
        ui->videoWidget->onPreviousBtnClicked();
    });

    m_key_fullScreen = new QShortcut(Qt::Key_F11, this);
    connect(m_key_fullScreen, &QShortcut::activated, this, [this]() {
        ui->titlebar->onFullscreenButtonClicked();
    });
}

void MainWidget::onOpenFileDlg()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("打开文件"),
        QStandardPaths::writableLocation(QStandardPaths::MoviesLocation),
        // QString(),
        tr("媒体文件 (*.mp3 *.mp4 *.avi *.mkv *.wav *.flac);;所有文件 (*.*)"));

    if (!filePath.isEmpty() && !onOpenFile(filePath)) {
        qWarning() << "file open failed.";
    }
}

void MainWidget::onOpenFolder()
{
    QString folderPath = QFileDialog::getExistingDirectory(this,
                                                           tr("打开文件夹"),
                                                           QString(),
                                                           QFileDialog::ShowDirsOnly
                                                               | QFileDialog::DontResolveSymlinks);

    if (!folderPath.isEmpty()) {
        // TODO: 处理打开文件夹的逻辑
        qDebug() << "Opening folder:" << folderPath;
    }
}

void MainWidget::onCloseToTray()
{
    hide();

    // 如果托盘图标存在，显示通知
    if (m_trayIcon && QSystemTrayIcon::supportsMessages()) {
        m_trayIcon->showMessage(tr("QWaveBox"),
                                tr("应用程序已最小化到系统托盘"),
                                QSystemTrayIcon::Information,
                                2000);
    }
}

void MainWidget::onOptions()
{
    // 显示选项对话框
    QMessageBox::information(this, tr("选项"), tr("选项功能尚未实现"));
}

void MainWidget::onAbout()
{
    // 显示关于对话框
    QMessageBox::about(this,
                       tr("关于 QWaveBox"),
                       tr("<h3>QWaveBox</h3>"
                          "<p>版本: 1.0</p>"
                          "<p>一个基于Qt开发的跨平台媒体播放器</p>"
                          "<p>Copyright 2025 QWaveBox Team</p>"));
}

void MainWidget::onQuitApplication()
{
    // 退出应用程序
    m_threadManager->stopPlay();
    QApplication::quit();
}

void MainWidget::onPlayTriggered()
{
    if (m_threadManager->isPlaying()) {
        m_threadManager->pausePlay();
    } else {
        m_threadManager->resumePlay();
    }
}
