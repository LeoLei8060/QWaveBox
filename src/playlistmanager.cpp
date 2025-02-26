#include "playlistmanager.h"

#include <QFileInfo>
#include <QUrl>

PlaylistManager::PlaylistManager(QObject *parent)
    : QObject(parent)
    , m_currentPlaylist(nullptr)
    , m_localMediaList(nullptr)
    , m_networkMediaList(nullptr)
    , m_favoriteList(nullptr)
    , m_currentIndex(-1) {
}

void PlaylistManager::setPlaylistWidgets(QListWidget *currentList, QListWidget *localList, 
                                       QListWidget *networkList, QListWidget *favoriteList) {
    m_currentPlaylist = currentList;
    m_localMediaList = localList;
    m_networkMediaList = networkList;
    m_favoriteList = favoriteList;
}

void PlaylistManager::addToCurrentPlaylist(const QString &filePath, const QString &title) {
    if (!m_currentPlaylist) {
        return;
    }
    
    QString displayTitle = title.isEmpty() ? getDisplayName(filePath) : title;
    addToList(m_currentPlaylist, filePath, displayTitle);
    
    // 添加到最近打开文件
    if (!m_recentFiles.contains(filePath)) {
        m_recentFiles.prepend(filePath);
        if (m_recentFiles.size() > MAX_RECENT_FILES) {
            m_recentFiles.removeLast();
        }
    } else {
        m_recentFiles.removeAll(filePath);
        m_recentFiles.prepend(filePath);
    }
}

void PlaylistManager::addToLocalList(const QString &filePath, const QString &title) {
    if (!m_localMediaList) {
        return;
    }
    
    QString displayTitle = title.isEmpty() ? getDisplayName(filePath) : title;
    addToList(m_localMediaList, filePath, displayTitle);
}

void PlaylistManager::addToNetworkList(const QString &url, const QString &title) {
    if (!m_networkMediaList) {
        return;
    }
    
    QString displayTitle = title.isEmpty() ? url : title;
    addToList(m_networkMediaList, url, displayTitle);
}

void PlaylistManager::addToFavoriteList(const QString &filePath, const QString &title) {
    if (!m_favoriteList) {
        return;
    }
    
    QString displayTitle = title.isEmpty() ? getDisplayName(filePath) : title;
    addToList(m_favoriteList, filePath, displayTitle);
}

void PlaylistManager::savePlaylist() {
    QSettings settings;
    
    // 保存最近文件
    settings.beginGroup("RecentFiles");
    settings.setValue("recentFiles", m_recentFiles);
    settings.endGroup();
    
    // 保存本地媒体列表
    settings.beginGroup("LocalList");
    QStringList localFiles;
    QStringList localTitles;
    if (m_localMediaList) {
        for (int i = 0; i < m_localMediaList->count(); ++i) {
            QListWidgetItem *item = m_localMediaList->item(i);
            localFiles.append(item->data(Qt::UserRole).toString());
            localTitles.append(item->text());
        }
    }
    settings.setValue("files", localFiles);
    settings.setValue("titles", localTitles);
    settings.endGroup();
    
    // 保存网络媒体列表
    settings.beginGroup("NetworkList");
    QStringList networkUrls;
    QStringList networkTitles;
    if (m_networkMediaList) {
        for (int i = 0; i < m_networkMediaList->count(); ++i) {
            QListWidgetItem *item = m_networkMediaList->item(i);
            networkUrls.append(item->data(Qt::UserRole).toString());
            networkTitles.append(item->text());
        }
    }
    settings.setValue("urls", networkUrls);
    settings.setValue("titles", networkTitles);
    settings.endGroup();
    
    // 保存收藏夹列表
    settings.beginGroup("FavoriteList");
    QStringList favoriteFiles;
    QStringList favoriteTitles;
    if (m_favoriteList) {
        for (int i = 0; i < m_favoriteList->count(); ++i) {
            QListWidgetItem *item = m_favoriteList->item(i);
            favoriteFiles.append(item->data(Qt::UserRole).toString());
            favoriteTitles.append(item->text());
        }
    }
    settings.setValue("files", favoriteFiles);
    settings.setValue("titles", favoriteTitles);
    settings.endGroup();
}

void PlaylistManager::loadPlaylist() {
    QSettings settings;
    
    // 加载最近文件
    settings.beginGroup("RecentFiles");
    m_recentFiles = settings.value("recentFiles").toStringList();
    settings.endGroup();
    
    // 加载本地媒体列表
    settings.beginGroup("LocalList");
    QStringList localFiles = settings.value("files").toStringList();
    QStringList localTitles = settings.value("titles").toStringList();
    if (m_localMediaList) {
        m_localMediaList->clear();
        for (int i = 0; i < localFiles.size(); ++i) {
            QString title = i < localTitles.size() ? localTitles[i] : getDisplayName(localFiles[i]);
            addToList(m_localMediaList, localFiles[i], title);
        }
    }
    settings.endGroup();
    
    // 加载网络媒体列表
    settings.beginGroup("NetworkList");
    QStringList networkUrls = settings.value("urls").toStringList();
    QStringList networkTitles = settings.value("titles").toStringList();
    if (m_networkMediaList) {
        m_networkMediaList->clear();
        for (int i = 0; i < networkUrls.size(); ++i) {
            QString title = i < networkTitles.size() ? networkTitles[i] : networkUrls[i];
            addToList(m_networkMediaList, networkUrls[i], title);
        }
    }
    settings.endGroup();
    
    // 加载收藏夹列表
    settings.beginGroup("FavoriteList");
    QStringList favoriteFiles = settings.value("files").toStringList();
    QStringList favoriteTitles = settings.value("titles").toStringList();
    if (m_favoriteList) {
        m_favoriteList->clear();
        for (int i = 0; i < favoriteFiles.size(); ++i) {
            QString title = i < favoriteTitles.size() ? favoriteTitles[i] : getDisplayName(favoriteFiles[i]);
            addToList(m_favoriteList, favoriteFiles[i], title);
        }
    }
    settings.endGroup();
}

void PlaylistManager::playItem(int index) {
    if (!m_currentPlaylist || index < 0 || index >= m_currentPlaylist->count()) {
        return;
    }
    
    m_currentIndex = index;
    QListWidgetItem *item = m_currentPlaylist->item(index);
    QString filePath = item->data(Qt::UserRole).toString();
    
    emit playlistItemChanged(filePath);
}

void PlaylistManager::playPrevious() {
    if (!m_currentPlaylist || m_currentPlaylist->count() == 0) {
        return;
    }
    
    if (m_currentIndex > 0) {
        playItem(m_currentIndex - 1);
    } else {
        // 循环到最后一个
        playItem(m_currentPlaylist->count() - 1);
    }
}

void PlaylistManager::playNext() {
    if (!m_currentPlaylist || m_currentPlaylist->count() == 0) {
        return;
    }
    
    if (m_currentIndex < m_currentPlaylist->count() - 1) {
        playItem(m_currentIndex + 1);
    } else {
        // 循环到第一个
        playItem(0);
    }
}

void PlaylistManager::removeItem(int index) {
    if (!m_currentPlaylist || index < 0 || index >= m_currentPlaylist->count()) {
        return;
    }
    
    delete m_currentPlaylist->takeItem(index);
    
    // 更新当前索引
    if (index == m_currentIndex) {
        m_currentIndex = -1;
    } else if (index < m_currentIndex) {
        m_currentIndex--;
    }
}

void PlaylistManager::clearPlaylist() {
    if (m_currentPlaylist) {
        m_currentPlaylist->clear();
        m_currentIndex = -1;
    }
}

void PlaylistManager::addToList(QListWidget *list, const QString &filePath, const QString &title) {
    if (!list) {
        return;
    }
    
    // 检查是否已经存在
    for (int i = 0; i < list->count(); ++i) {
        if (list->item(i)->data(Qt::UserRole).toString() == filePath) {
            return;
        }
    }
    
    QListWidgetItem *item = new QListWidgetItem(title);
    item->setData(Qt::UserRole, filePath);
    list->addItem(item);
}

QString PlaylistManager::getDisplayName(const QString &filePath) {
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists()) {
        return fileInfo.fileName();
    }
    
    QUrl url(filePath);
    if (url.isValid()) {
        QString path = url.path();
        QFileInfo urlFileInfo(path);
        if (!urlFileInfo.fileName().isEmpty()) {
            return urlFileInfo.fileName();
        }
        return url.host();
    }
    
    return filePath;
}
