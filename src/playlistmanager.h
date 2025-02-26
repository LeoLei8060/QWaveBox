#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <QObject>
#include <QListWidget>
#include <QSettings>
#include <QUrl>
#include <QStringList>

class PlaylistManager : public QObject {
    Q_OBJECT
    
public:
    explicit PlaylistManager(QObject *parent = nullptr);
    
    void setPlaylistWidgets(QListWidget *currentList, QListWidget *localList, 
                           QListWidget *networkList, QListWidget *favoriteList);
    
    void addToCurrentPlaylist(const QString &filePath, const QString &title);
    void addToLocalList(const QString &filePath, const QString &title);
    void addToNetworkList(const QString &url, const QString &title);
    void addToFavoriteList(const QString &filePath, const QString &title);
    
    void savePlaylist();
    void loadPlaylist();
    
public slots:
    void playItem(int index);
    void playPrevious();
    void playNext();
    void removeItem(int index);
    void clearPlaylist();
    
signals:
    void playlistItemChanged(const QString &filePath);
    
private:
    void addToList(QListWidget *list, const QString &filePath, const QString &title);
    QString getDisplayName(const QString &filePath);
    
    QListWidget *m_currentPlaylist;
    QListWidget *m_localMediaList;
    QListWidget *m_networkMediaList;
    QListWidget *m_favoriteList;
    
    int m_currentIndex;
    QStringList m_recentFiles;
    
    const int MAX_RECENT_FILES = 20;
};

#endif // PLAYLISTMANAGER_H
