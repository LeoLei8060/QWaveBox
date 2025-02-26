#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QListWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QMenu>
#include <QAction>

#include "videoplayer.h"
#include "playlistmanager.h"
#include "mediacontrolwidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
    void openFile(const QString &filePath);
    
protected:
    void closeEvent(QCloseEvent *event) override;
    
private slots:
    void onOpen();
    void onExit();
    void onAbout();
    void onPlaylistItemDoubleClicked(QListWidgetItem *item);
    void onMediaStateChanged(VideoPlayer::MediaState state);
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void onMediaStatusChanged(VideoPlayer::MediaStatus status);
    
private:
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void createPlayerLayout();
    void saveSettings();
    void loadSettings();
    
    Ui::MainWindow *ui;
    
    // 核心组件
    VideoPlayer *m_videoPlayer;
    PlaylistManager *m_playlistManager;
    MediaControlWidget *m_mediaControl;
    
    // Tab页
    QTabWidget *m_tabWidget;
    QListWidget *m_currentPlaylist;
    QListWidget *m_localMediaList;
    QListWidget *m_networkMediaList;
    QListWidget *m_favoriteList;
    
    // 菜单和动作
    QMenu *m_fileMenu;
    QMenu *m_editMenu;
    QMenu *m_playbackMenu;
    QMenu *m_viewMenu;
    QMenu *m_helpMenu;
    
    QAction *m_openAction;
    QAction *m_exitAction;
    QAction *m_playAction;
    QAction *m_pauseAction;
    QAction *m_stopAction;
    QAction *m_previousAction;
    QAction *m_nextAction;
    QAction *m_fullscreenAction;
    QAction *m_aboutAction;
};

#endif // MAINWINDOW_H
