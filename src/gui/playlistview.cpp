#include "playlistview.h"
#include "playlistmodel.h"
#include <QMouseEvent>

PlayListView::PlayListView(QWidget *parent)
    : QListView(parent)
{
    setSelectionMode(QAbstractItemView::SingleSelection);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

QString PlayListView::getSelectedPath()
{
    if (!selectionMode())
        return "";
    QModelIndexList indexes = selectionModel()->selectedIndexes();
    for (auto &index : indexes) {
        if (index.isValid()) {
            return index.data(Qt::UserRole).toString();
        }
    }
    return "";
}

bool PlayListView::addItem(const QString &text)
{
    if (!model())
        return false;
    return static_cast<PlayListModel *>(model())->addItem(text);
}

void PlayListView::moveSelectedUp()
{
    QModelIndexList indexes = selectionModel()->selectedIndexes();
    for (auto &index : indexes) {
        if (model())
            static_cast<PlayListModel *>(model())->moveUp(index.row());
    }
}

void PlayListView::moveSelectedDown()
{
    QModelIndexList indexes = selectionModel()->selectedIndexes();
    // 需要倒序处理防止位置变化
    std::sort(indexes.begin(), indexes.end(), [](const QModelIndex &a, const QModelIndex &b) {
        return a.row() > b.row();
    });
    for (auto &index : indexes) {
        if (model())
            static_cast<PlayListModel *>(model())->moveDown(index.row());
    }
}

void PlayListView::moveToTop()
{
    QModelIndexList indexes = selectionModel()->selectedIndexes();
    for (auto &index : indexes) {
        if (model())
            static_cast<PlayListModel *>(model())->moveToTop(index.row());
    }
}

void PlayListView::moveToBottom()
{
    QModelIndexList indexes = selectionModel()->selectedIndexes();
    // 需要倒序处理防止位置变化
    std::sort(indexes.begin(), indexes.end(), [](const QModelIndex &a, const QModelIndex &b) {
        return a.row() > b.row();
    });
    for (auto &index : indexes) {
        if (model())
            static_cast<PlayListModel *>(model())->moveToBottom(index.row());
    }
}

void PlayListView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {
        emit sigFileDoubleClicked(index.data(Qt::UserRole).toString());
    }
    QListView::mouseDoubleClickEvent(event);
}
