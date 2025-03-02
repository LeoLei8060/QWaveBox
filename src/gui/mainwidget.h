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
    // 窗口边缘类型枚举
    enum class EdgeType {
        None = 0,
        Left = 1,
        Top = 2,
        Right = 3,
        Bottom = 4,
        TopLeft = 5,
        TopRight = 6,
        BottomLeft = 7,
        BottomRight = 8
    };

    explicit MainWidget(QWidget *parent = nullptr);
    ~MainWidget();

protected:
    void changeEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    Ui::MainWidget *ui;
    bool m_isMaximized = false;
    bool m_resizing = false;
    QPoint m_dragPos;
    EdgeType m_dragEdge = EdgeType::None;
    int m_borderWidth = 1; // Width of the resize area
};

#endif // MAINWIDGET_H
