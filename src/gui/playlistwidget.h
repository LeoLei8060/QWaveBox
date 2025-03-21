#ifndef PLAYLISTWIDGET_H
#define PLAYLISTWIDGET_H

#include <QPushButton>
#include <QWidget>

namespace Ui {
class PlaylistWidget;
}

class PlayListModel;

class PlaylistWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlaylistWidget(QWidget *parent = nullptr);
    ~PlaylistWidget();

    void addFileToDefaultList(const QString &file);

    // 播放选中项（提供给快捷键使用）
    void playSelected();

signals:
    void sigOpenFile(const QString &);

private:
    void setupTabWidget();

    void setupDefaultList();
    void setupComputerList();

private:
    Ui::PlaylistWidget *ui;

    QPushButton   *m_addTabBtn;
    PlayListModel *m_defaultModel;
};

#endif // PLAYLISTWIDGET_H
