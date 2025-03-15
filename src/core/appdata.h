#ifndef APPDATA_H
#define APPDATA_H

#include <obj_conv/reflex_format.hpp>
#include <QList>

struct PlayFile
{
    QString filename_;
    QString filepath_;

    REFLEX_BIND(A(filename_, "filename"), A(filepath_, "filepath"))
};

class Album
{
public:
    Album() = default;
    Album(const QString &name);

    QString getAlbumName() const { return m_name; }
    void    setAlbumName(const QString &name) { m_name = name; }

    QList<PlayFile> getPlayfiles() const { return m_files; }
    void            addPlayFile(const PlayFile &file) { m_files.append(file); }
    void            deletePlayFile(const QString &filename);

private:
    QString         m_name{"默认专辑"};
    QList<PlayFile> m_files;

    REFLEX_BIND(A(m_name, "name"), A(m_files, "files"))
};

class AppData
{
public:
    AppData();

    QString getDefaultAlbumName() { return m_defaultAlbum.getAlbumName(); }
    Album   getDefaultAlbum() const { return m_defaultAlbum; }
    void    addPlayFileToDefAlbum(const PlayFile &file);
    void    deletePlayFileFromDefAlbum(const QString &filename);
    void    addPlayFileToDefAlbum(const QString &filepath);

    QList<Album> getCustomAlbums() const { return m_customAlbums; }
    void         addCustomAlbum(const QString &albumName);
    void         addPlayFileToCusAlbum(const QString &albumName, const PlayFile &file);
    void         deletePlayFileFromCusAlbum(const QString &albumName, const QString &filename);

    int  getVolume() const { return m_volume; }
    void setVolume(int volume) { m_volume = volume; }

    bool isMute() const { return m_isMute; }
    void setMute(bool isMute) { m_isMute = isMute; }

private:
    Album        m_defaultAlbum;
    QList<Album> m_customAlbums;
    int          m_volume{50};
    bool         m_isMute{false};

    REFLEX_BIND(A(m_defaultAlbum, "defaultAlbum"),
                A(m_customAlbums, "customAlbums"),
                A(m_volume, "volume"),
                A(m_isMute, "isMute"))
};

#endif // APPDATA_H
