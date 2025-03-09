#ifndef FILELISTVIEW_H
#define FILELISTVIEW_H

#include "filelistmodel.h"
#include <QListView>

class FileListView : public QListView
{
    Q_OBJECT

public:
    explicit FileListView(QWidget *parent = nullptr);

    void    setDirectory(const QString &path);
    QString currentDirectory() const;

signals:
    void sigDirectoryChanged(const QString &path);
    void sigFileSelected(const QString &filePath);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    FileListModel *m_model;
};

#endif // FILELISTVIEW_H
