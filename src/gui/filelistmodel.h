#ifndef FILELISTMODEL_H
#define FILELISTMODEL_H

#include <QAbstractListModel>
#include <QDir>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QList>

class FileListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit FileListModel(QObject *parent = nullptr);
    ~FileListModel();

    int      rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void    setDirectory(const QString &path);
    QString currentDirectory() const;
    bool    isDirectory(const QModelIndex &index) const;
    QString filePath(const QModelIndex &index) const;

private:
    void  refreshFileList();
    bool  isMediaFile(const QFileInfo &fileInfo) const;
    QIcon getFileIcon(const QFileInfo &fileInfo) const;
    bool  shouldShowUpDirectory() const; // 判断是否显示"返回上一级"选项

    QDir               m_currentDir;
    QList<QFileInfo>   m_fileList;
    QFileIconProvider *m_iconProvider;
    QString            m_previousDir; // 上一层目录
};

#endif // FILELISTMODEL_H
