#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QFileInfo>
#include <QWidget>

struct PlayListItem
{
    QString   fileName;
    QString   filePath;
    qint64    size;
    QDateTime lastModified;

    PlayListItem(const QFileInfo &info)
        : fileName(info.fileName())
        , filePath(info.absoluteFilePath())
        , size(info.size())
        , lastModified(info.lastModified())
    {}
};

class PlayListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum SortMode { Name, Ext, Size, Time };
    enum SortOrder { Ascending, Descending };

    explicit PlayListModel(QObject *parent = nullptr);

    // 基本模型接口
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // 数据操作
    bool addItem(const QString &text);

    void removeRows(const QModelIndexList &indexes);

    // 排序接口
    void sort(SortMode mode, SortOrder order);

    // 移动操作
    void moveUp(int row);

    void moveDown(int row);

    void moveToTop(int row);

    void moveToBottom(int row);

    // 查找功能
    QModelIndex find(const QString &text);

private:
    void performSort();

    QList<PlayListItem> m_items;
    SortMode            m_sortMode;
    SortOrder           m_sortOrder;
};

#endif // PLAYLISTMODEL_H
