#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    
    ui->setupUi(this);
    
    // 初始化核心组件
    m_videoPlayer = new VideoPlayer(this);
    m_playlistManager = new PlaylistManager(this);
    m_mediaControl = new MediaControlWidget(this);
    
    // 连接信号和槽
    connect(m_videoPlayer, &VideoPlayer::mediaStateChanged, this, &MainWindow::onMediaStateChanged);
    connect(m_videoPlayer, &VideoPlayer::positionChanged, this, &MainWindow::onPositionChanged);
    connect(m_videoPlayer, &VideoPlayer::durationChanged, this, &MainWindow::onDurationChanged);
    connect(m_videoPlayer, &VideoPlayer::mediaStatusChanged, this, &MainWindow::onMediaStatusChanged);
    
    connect(m_mediaControl, &MediaControlWidget::play, m_videoPlayer, &VideoPlayer::play);
    connect(m_mediaControl, &MediaControlWidget::pause, m_videoPlayer, &VideoPlayer::pause);
    connect(m_mediaControl, &MediaControlWidget::stop, m_videoPlayer, &VideoPlayer::stop);
    connect(m_mediaControl, &MediaControlWidget::previous, m_playlistManager, &PlaylistManager::playPrevious);
    connect(m_mediaControl, &MediaControlWidget::next, m_playlistManager, &PlaylistManager::playNext);
    connect(m_mediaControl, &MediaControlWidget::positionChanged, m_videoPlayer, &VideoPlayer::setPosition);
    connect(m_mediaControl, &MediaControlWidget::volumeChanged, m_videoPlayer, &VideoPlayer::setVolume);
    connect(m_mediaControl, &MediaControlWidget::speedChanged, m_videoPlayer, &VideoPlayer::setPlaybackRate);
    
    // 创建UI界面
    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    createPlayerLayout();
    
    // 加载设置
    loadSettings();
    
    // 设置窗口标题和大小
    setWindowTitle(tr("Video Player"));
    resize(800, 600);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::openFile(const QString &filePath) {
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists()) {
        m_videoPlayer->setMedia(filePath);
        m_playlistManager->addToCurrentPlaylist(filePath, fileInfo.fileName());
        statusBar()->showMessage(tr("Opened: %1").arg(filePath));
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    saveSettings();
    event->accept();
}

void MainWindow::onOpen() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Media"),
        QStandardPaths::standardLocations(QStandardPaths::MoviesLocation).value(0, QDir::homePath()),
        tr("Media Files (*.mp4 *.mkv *.avi *.mov *.mp3 *.wav *.flac);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        openFile(fileName);
    }
}

void MainWindow::onExit() {
    close();
}

void MainWindow::onAbout() {
    QMessageBox::about(this, tr("About Video Player"),
        tr("A simple video player based on Qt and FFmpeg.\n"
           "Developed for educational purposes."));
}

void MainWindow::onPlaylistItemDoubleClicked(QListWidgetItem *item) {
    QString filePath = item->data(Qt::UserRole).toString();
    if (!filePath.isEmpty()) {
        m_videoPlayer->setMedia(filePath);
        m_videoPlayer->play();
    }
}

void MainWindow::onMediaStateChanged(VideoPlayer::MediaState state) {
    m_mediaControl->updatePlaybackState(state == VideoPlayer::PlayingState);
}

void MainWindow::onPositionChanged(qint64 position) {
    m_mediaControl->updatePosition(position);
    
    // 更新状态栏显示当前播放时间
    QTime currentTime(0, 0);
    currentTime = currentTime.addMSecs(position);
    QTime totalTime(0, 0);
    totalTime = totalTime.addMSecs(m_videoPlayer->duration());
    statusBar()->showMessage(
        tr("Time: %1 / %2")
            .arg(currentTime.toString("hh:mm:ss"))
            .arg(totalTime.toString("hh:mm:ss")));
}

void MainWindow::onDurationChanged(qint64 duration) {
    m_mediaControl->updateDuration(duration);
}

void MainWindow::onMediaStatusChanged(VideoPlayer::MediaStatus status) {
    switch (status) {
        case VideoPlayer::LoadingMedia:
            statusBar()->showMessage(tr("Loading media..."));
            break;
        case VideoPlayer::LoadedMedia:
            statusBar()->showMessage(tr("Media loaded."));
            break;
        case VideoPlayer::InvalidMedia:
            statusBar()->showMessage(tr("Invalid media."));
            QMessageBox::warning(this, tr("Error"), tr("Cannot play the selected media file."));
            break;
        default:
            break;
    }
}

void MainWindow::createActions() {
    // 文件菜单动作
    m_openAction = new QAction(tr("&Open"), this);
    m_openAction->setShortcut(QKeySequence::Open);
    m_openAction->setStatusTip(tr("Open a media file"));
    connect(m_openAction, &QAction::triggered, this, &MainWindow::onOpen);
    
    m_exitAction = new QAction(tr("E&xit"), this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setStatusTip(tr("Exit the application"));
    connect(m_exitAction, &QAction::triggered, this, &MainWindow::onExit);
    
    // 播放控制动作
    m_playAction = new QAction(tr("&Play"), this);
    m_playAction->setShortcut(Qt::Key_Space);
    m_playAction->setStatusTip(tr("Play the media"));
    connect(m_playAction, &QAction::triggered, m_videoPlayer, &VideoPlayer::play);
    
    m_pauseAction = new QAction(tr("P&ause"), this);
    m_pauseAction->setShortcut(Qt::Key_Space);
    m_pauseAction->setStatusTip(tr("Pause the media"));
    connect(m_pauseAction, &QAction::triggered, m_videoPlayer, &VideoPlayer::pause);
    
    m_stopAction = new QAction(tr("&Stop"), this);
    m_stopAction->setShortcut(Qt::Key_S);
    m_stopAction->setStatusTip(tr("Stop the media"));
    connect(m_stopAction, &QAction::triggered, m_videoPlayer, &VideoPlayer::stop);
    
    m_previousAction = new QAction(tr("Pre&vious"), this);
    m_previousAction->setShortcut(Qt::Key_Left);
    m_previousAction->setStatusTip(tr("Play the previous item"));
    connect(m_previousAction, &QAction::triggered, m_playlistManager, &PlaylistManager::playPrevious);
    
    m_nextAction = new QAction(tr("&Next"), this);
    m_nextAction->setShortcut(Qt::Key_Right);
    m_nextAction->setStatusTip(tr("Play the next item"));
    connect(m_nextAction, &QAction::triggered, m_playlistManager, &PlaylistManager::playNext);
    
    // 视图动作
    m_fullscreenAction = new QAction(tr("&Fullscreen"), this);
    m_fullscreenAction->setShortcut(Qt::Key_F);
    m_fullscreenAction->setStatusTip(tr("Toggle fullscreen mode"));
    m_fullscreenAction->setCheckable(true);
    connect(m_fullscreenAction, &QAction::toggled, this, [this](bool checked) {
        if (checked) {
            showFullScreen();
        } else {
            showNormal();
        }
    });
    
    // 帮助菜单动作
    m_aboutAction = new QAction(tr("&About"), this);
    m_aboutAction->setStatusTip(tr("Show the application's About box"));
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::createMenus() {
    // 创建菜单栏
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_openAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exitAction);
    
    m_playbackMenu = menuBar()->addMenu(tr("&Playback"));
    m_playbackMenu->addAction(m_playAction);
    m_playbackMenu->addAction(m_pauseAction);
    m_playbackMenu->addAction(m_stopAction);
    m_playbackMenu->addSeparator();
    m_playbackMenu->addAction(m_previousAction);
    m_playbackMenu->addAction(m_nextAction);
    
    m_viewMenu = menuBar()->addMenu(tr("&View"));
    m_viewMenu->addAction(m_fullscreenAction);
    
    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_aboutAction);
}

void MainWindow::createToolBars() {
    QToolBar *fileToolBar = addToolBar(tr("File"));
    fileToolBar->addAction(m_openAction);
    
    QToolBar *playbackToolBar = addToolBar(tr("Playback"));
    playbackToolBar->addAction(m_playAction);
    playbackToolBar->addAction(m_pauseAction);
    playbackToolBar->addAction(m_stopAction);
    playbackToolBar->addAction(m_previousAction);
    playbackToolBar->addAction(m_nextAction);
}

void MainWindow::createStatusBar() {
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::createPlayerLayout() {
    // 创建主分割器
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    
    // 创建左侧视频播放区域
    QWidget *videoContainer = new QWidget(splitter);
    QVBoxLayout *videoLayout = new QVBoxLayout(videoContainer);
    videoLayout->addWidget(m_videoPlayer->videoWidget());
    videoLayout->addWidget(m_mediaControl);
    
    // 创建右侧播放列表区域
    m_tabWidget = new QTabWidget(splitter);
    
    // 创建各个标签页
    m_currentPlaylist = new QListWidget();
    m_localMediaList = new QListWidget();
    m_networkMediaList = new QListWidget();
    m_favoriteList = new QListWidget();
    
    // 添加到标签页
    m_tabWidget->addTab(m_currentPlaylist, tr("Current Playlist"));
    m_tabWidget->addTab(m_localMediaList, tr("Local Media"));
    m_tabWidget->addTab(m_networkMediaList, tr("Network"));
    m_tabWidget->addTab(m_favoriteList, tr("Favorites"));
    
    // 连接播放列表的双击事件
    connect(m_currentPlaylist, &QListWidget::itemDoubleClicked, this, &MainWindow::onPlaylistItemDoubleClicked);
    connect(m_localMediaList, &QListWidget::itemDoubleClicked, this, &MainWindow::onPlaylistItemDoubleClicked);
    connect(m_networkMediaList, &QListWidget::itemDoubleClicked, this, &MainWindow::onPlaylistItemDoubleClicked);
    connect(m_favoriteList, &QListWidget::itemDoubleClicked, this, &MainWindow::onPlaylistItemDoubleClicked);
    
    // 添加到分割器
    splitter->addWidget(videoContainer);
    splitter->addWidget(m_tabWidget);
    
    // 设置默认大小
    splitter->setSizes(QList<int>() << 600 << 200);
    
    // 设置为中央部件
    setCentralWidget(splitter);
    
    // 设置播放列表管理器
    m_playlistManager->setPlaylistWidgets(m_currentPlaylist, m_localMediaList, 
                                        m_networkMediaList, m_favoriteList);
}

void MainWindow::saveSettings() {
    QSettings settings;
    settings.beginGroup("MainWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("volume", m_videoPlayer->volume());
    settings.endGroup();
    
    // 保存播放列表
    m_playlistManager->savePlaylist();
}

void MainWindow::loadSettings() {
    QSettings settings;
    settings.beginGroup("MainWindow");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    m_videoPlayer->setVolume(settings.value("volume", 50).toInt());
    settings.endGroup();
    
    // 加载播放列表
    m_playlistManager->loadPlaylist();
}
