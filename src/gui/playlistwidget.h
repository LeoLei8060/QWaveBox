#ifndef PLAYLISTWIDGET_H
#define PLAYLISTWIDGET_H

#include <QPushButton>
#include <QWidget>

namespace Ui {
class PlaylistWidget;
}

class PlaylistWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlaylistWidget(QWidget *parent = nullptr);
    ~PlaylistWidget();

signals:
    void sigOpenFile(const QString &);

private:
    void setupTabWidget();

    void setupComputerList();

private:
    Ui::PlaylistWidget *ui;

    QPushButton *m_addTabBtn;
};

#endif // PLAYLISTWIDGET_H
