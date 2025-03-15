#include "appdata.h"
#include <QFileInfo>

AppData::AppData() {}

void AppData::addPlayFileToDefAlbum(const PlayFile &file)
{
    m_defaultAlbum.addPlayFile(file);
}

void AppData::deletePlayFileFromDefAlbum(const QString &filename)
{
    m_defaultAlbum.deletePlayFile(filename);
}

void AppData::addPlayFileToDefAlbum(const QString &filepath)
{
    QFileInfo info(filepath);
    if (info.isFile()) {
        m_defaultAlbum.addPlayFile({info.fileName(), filepath});
    }
}

void AppData::addCustomAlbum(const QString &albumName)
{
    m_customAlbums.append(Album(albumName));
}

void AppData::addPlayFileToCusAlbum(const QString &albumName, const PlayFile &file)
{
    for (Album &album : m_customAlbums) {
        if (album.getAlbumName() == albumName) {
            album.addPlayFile(file);
            break;
        }
    }
}

void AppData::deletePlayFileFromCusAlbum(const QString &albumName, const QString &filename)
{
    for (Album &album : m_customAlbums) {
        if (album.getAlbumName() == albumName) {
            album.deletePlayFile(filename);
            break;
        }
    }
}

Album::Album(const QString &name)
    : m_name(name)
{}

void Album::deletePlayFile(const QString &filename)
{
    for (auto iter = m_files.begin(); iter != m_files.end();) {
        if ((*iter).filename_ == filename)
            iter = m_files.erase(iter);
        else
            ++iter;
    }
}
