#include "playlistmodel.h"

PlayListModel::PlayListModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_sortMode(Name)
    , m_sortOrder(Ascending)
{}

int PlayListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.count();
}

QVariant PlayListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.count())
        return QVariant();

    const PlayListItem &item = m_items[index.row()];
    if (role == Qt::DisplayRole)
        return item.fileName;
    else if (role == Qt::UserRole)
        return item.filePath;
    return QVariant();
}

void PlayListModel::addItem(const QString &text)
{
    QFileInfo info(text);
    if (!info.exists())
        return;

    beginInsertRows(QModelIndex(), m_items.count(), m_items.count());
    m_items.append(PlayListItem(info));
    endInsertRows();
}

void PlayListModel::removeRows(const QModelIndexList &indexes)
{
    QList<int> rows;
    foreach (const QModelIndex &index, indexes)
        rows << index.row();

    std::sort(rows.begin(), rows.end(), std::greater<int>());
    foreach (int row, rows) {
        beginRemoveRows(QModelIndex(), row, row);
        m_items.removeAt(row);
        endRemoveRows();
    }
}

void PlayListModel::sort(SortMode mode, SortOrder order)
{
    m_sortMode = mode;
    m_sortOrder = order;
    performSort();
}

void PlayListModel::moveUp(int row)
{
    if (row <= 0)
        return;
    beginMoveRows(QModelIndex(), row, row, QModelIndex(), row - 1);
    m_items.swapItemsAt(row, row - 1);
    endMoveRows();
}

void PlayListModel::moveDown(int row)
{
    if (row >= m_items.count() - 1)
        return;
    beginMoveRows(QModelIndex(), row, row, QModelIndex(), row + 2);
    m_items.swapItemsAt(row, row + 1);
    endMoveRows();
}

void PlayListModel::moveToTop(int row)
{
    if (row <= 0)
        return;
    beginMoveRows(QModelIndex(), row, row, QModelIndex(), 0);
    m_items.move(row, 0);
    endMoveRows();
}

void PlayListModel::moveToBottom(int row)
{
    if (row >= m_items.count() - 1)
        return;
    beginMoveRows(QModelIndex(), row, row, QModelIndex(), m_items.count());
    m_items.move(row, m_items.count() - 1);
    endMoveRows();
}

QModelIndex PlayListModel::find(const QString &text)
{
    for (int i = 0; i < m_items.count(); ++i) {
        if (m_items[i].fileName.contains(text, Qt::CaseInsensitive)) {
            return index(i);
        }
    }
    return QModelIndex();
}

void PlayListModel::performSort()
{
    beginResetModel();
    std::sort(m_items.begin(),
              m_items.end(),
              [this](const PlayListItem &a, const PlayListItem &b) {
                  bool lessThan = false;
                  switch (m_sortMode) {
                  case Name:
                      // 修改为不区分大小写的比较
                      lessThan = a.fileName.compare(b.fileName, Qt::CaseInsensitive) < 0;
                      break;
                  case Ext:
                      // 扩展名也不区分大小写
                      lessThan = QFileInfo(a.fileName)
                                     .suffix()
                                     .compare(QFileInfo(b.fileName).suffix(),
                                              Qt::CaseInsensitive)
                                 < 0;
                      break;
                  case Size:
                      lessThan = a.size < b.size;
                      break;
                  case Time:
                      lessThan = a.lastModified < b.lastModified;
                      break;
                  }
                  return m_sortOrder == Ascending ? lessThan : !lessThan;
              });
    endResetModel();
}
