#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>

namespace Ui {
class MainWidget;
}

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget(QWidget *parent = nullptr);
    ~MainWidget();

protected:
    void changeEvent(QEvent *event) override;

private:
    Ui::MainWidget *ui;
    bool m_isMaximized = false;
};

#endif // MAINWIDGET_H
