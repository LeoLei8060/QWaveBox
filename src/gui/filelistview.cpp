#include "filelistview.h"
#include <QDebug>
#include <QMouseEvent>

FileListView::FileListView(QWidget *parent)
    : QListView(parent)
{
    m_model = new FileListModel(this);
    setModel(m_model);

    // 设置视图属性
    setSelectionMode(QAbstractItemView::SingleSelection);

    // 设置图标大小为更合适的尺寸，以便更好地显示系统图标
    setIconSize(QSize(24, 24));

    // 添加更多视图设置，使其看起来更像系统资源管理器
    setUniformItemSizes(true); // 项目大小统一，提高性能
    setSpacing(1);             // 项目间距
}

void FileListView::setDirectory(const QString &path)
{
    m_model->setDirectory(path);
    emit sigDirectoryChanged(path);
}

QString FileListView::currentDirectory() const
{
    return m_model->currentDirectory();
}

QString FileListView::getSelectedPath()
{
    QModelIndexList indexes = selectionModel()->selectedIndexes();
    for (auto &index : indexes) {
        if (index.isValid() && !m_model->isDirectory(index)) {
            return m_model->filePath(index);
        }
    }
    return "";
}

void FileListView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QListView::mouseDoubleClickEvent(event);

    QModelIndex index = indexAt(event->pos());
    if (!index.isValid())
        return;

    if (m_model->isDirectory(index)) {
        // 如果是目录，进入该目录
        setDirectory(m_model->filePath(index));
    } else {
        // 如果是文件，发出信号
        emit sigFileSelected(m_model->filePath(index));
    }
}
