#include "filelistmodel.h"
#include <QApplication>
#include <QDebug>
#include <QStorageInfo>
#include <QStyle>

#define ROOT_NAME tr("::MyComputer")

FileListModel::FileListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // 初始化图标提供器，设置选项以获取系统图标
    m_iconProvider = new QFileIconProvider();

    // 初始化为系统视频目录
    QString videoPath = QDir::homePath() + "/Videos";

    // 如果系统视频目录不存在，则使用用户目录
    if (!QDir(videoPath).exists()) {
        videoPath = QDir::homePath();
    }

    setDirectory(videoPath);
}

FileListModel::~FileListModel()
{
    delete m_iconProvider;
}

int FileListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    // 文件列表数量加1（为了显示返回上一层的条目），但如果是在驱动器根目录下，依然显示返回上层选项
    return m_fileList.size() + (shouldShowUpDirectory() ? 1 : 0);
}

QVariant FileListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount())
        return QVariant();

    // 处理“此电脑”下的驱动器显示
    if (m_currentDir.absolutePath() == ROOT_NAME) {
        const QFileInfo &driveInfo = m_fileList.at(index.row());
        switch (role) {
        case Qt::DisplayRole:
            return driveInfo.absolutePath();
        case Qt::DecorationRole:
            return m_iconProvider->icon(QFileIconProvider::Drive);
        default:
            return QVariant();
        }
    }

    // 上一级目录显示
    if (index.row() == 0 && shouldShowUpDirectory()) {
        switch (role) {
        case Qt::DisplayRole:
            return "...";
        case Qt::DecorationRole:
            // 使用系统标准的文件夹图标
            return m_iconProvider->icon(QFileIconProvider::Folder);
        case Qt::ToolTipRole:
            return tr("上一级目录");
        default:
            return QVariant();
        }
    }

    // 实际文件和文件夹的展示
    int              actualRow = index.row() - (shouldShowUpDirectory() ? 1 : 0);
    const QFileInfo &fileInfo = m_fileList.at(actualRow);

    switch (role) {
    case Qt::DisplayRole:
        return fileInfo.fileName();
    case Qt::DecorationRole:
        // 获取与系统资源管理器一致的图标
        return getFileIcon(fileInfo);
    case Qt::ToolTipRole:
        return fileInfo.filePath();
    default:
        return QVariant();
    }
}

void FileListModel::setDirectory(const QString &path)
{
    beginResetModel();

    // 保存当前目录为上一级
    if (m_currentDir.exists()) {
        m_previousDir = m_currentDir.absolutePath();
    }

    m_currentDir.setPath(path);
    refreshFileList();

    endResetModel();
}

QString FileListModel::currentDirectory() const
{
    return m_currentDir.absolutePath();
}

bool FileListModel::isDirectory(const QModelIndex &index) const
{
    if (!index.isValid())
        return false;

    // 上一级目录
    if (index.row() == 0 && shouldShowUpDirectory()) {
        return true;
    }

    int actualRow = index.row() - (shouldShowUpDirectory() ? 1 : 0);
    if (actualRow < 0 || actualRow >= m_fileList.size())
        return false;

    return m_fileList.at(actualRow).isDir();
}

QString FileListModel::filePath(const QModelIndex &index) const
{
    if (!index.isValid())
        return QString();

    // 上一级目录
    if (index.row() == 0 && shouldShowUpDirectory()) {
        QDir parentDir(m_currentDir);

        // 检查当前是否是盘符根目录
        QStorageInfo storageInfo(m_currentDir.absolutePath());
        bool         isInDriveRoot = (m_currentDir.absolutePath() == storageInfo.rootPath());

        if (isInDriveRoot) {
            // 在Windows中返回"计算机"特殊文件夹
            return ROOT_NAME;
        } else {
            // 普通目录，正常返回上一级
            parentDir.cdUp();
            return parentDir.absolutePath();
        }
    }

    int actualRow = index.row() - (shouldShowUpDirectory() ? 1 : 0);
    if (actualRow < 0 || actualRow >= m_fileList.size())
        return QString();

    return m_fileList.at(actualRow).absoluteFilePath();
}

void FileListModel::refreshFileList()
{
    m_fileList.clear();

    if (m_currentDir.absolutePath() == ROOT_NAME) {
        // 列出所有磁盘驱动器
        QList<QStorageInfo> drives = QStorageInfo::mountedVolumes();
        for (const QStorageInfo &drive : drives) {
            if (drive.isValid() && !drive.rootPath().isEmpty()) {
                m_fileList.append(QFileInfo(drive.rootPath()));
            }
        }
        return;
    }

    // 设置文件过滤器
    QDir::Filters filters = QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot;

    // 获取所有文件和文件夹
    QFileInfoList allFiles = m_currentDir.entryInfoList(filters, QDir::DirsFirst);

    // 过滤出媒体文件和文件夹
    for (const QFileInfo &fileInfo : allFiles) {
        if (fileInfo.isDir() || isMediaFile(fileInfo)) {
            m_fileList.append(fileInfo);
        }
    }
}

bool FileListModel::isMediaFile(const QFileInfo &fileInfo) const
{
    if (fileInfo.isDir())
        return true;

    // 常见音视频文件扩展名
    static QStringList mediaExtensions = {// 视频格式
                                          "mp4",
                                          "avi",
                                          "mkv",
                                          "mov",
                                          "wmv",
                                          "flv",
                                          "webm",
                                          "m4v",
                                          "mpg",
                                          "mpeg",
                                          "3gp",
                                          // 音频格式
                                          "mp3",
                                          "wav",
                                          "flac",
                                          "aac",
                                          "ogg",
                                          "wma",
                                          "m4a",
                                          "opus"};

    QString ext = fileInfo.suffix().toLower();
    return mediaExtensions.contains(ext);
}

QIcon FileListModel::getFileIcon(const QFileInfo &fileInfo) const
{
    // 使用QFileIconProvider获取与系统一致的图标
    return m_iconProvider->icon(fileInfo);
}

bool FileListModel::shouldShowUpDirectory() const
{
    if (m_currentDir.absolutePath() == ROOT_NAME) {
        return false;
    }
    // 判断是否为驱动器根目录（例如C:/）
    QStorageInfo storageInfo(m_currentDir.absolutePath());
    bool         isInDriveRoot = (m_currentDir.absolutePath() == storageInfo.rootPath());

    // 如果不是系统根目录或者是驱动器根目录，则显示返回上层选项
    return (m_currentDir.path() != QDir::rootPath() || isInDriveRoot);
}
