#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include <QListView>

class PlayListView : public QListView
{
    Q_OBJECT
public:
    explicit PlayListView(QWidget *parent = nullptr);

    void addItem(const QString &text);

    void moveSelectedUp();
    void moveSelectedDown();
    void moveToTop();
    void moveToBottom();

signals:
    void sigFileDoubleClicked(const QString &filePath);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
};

#endif // PLAYLISTVIEW_H
