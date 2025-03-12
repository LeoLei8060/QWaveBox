#ifndef CLICKMOVABLESLIDER_H
#define CLICKMOVABLESLIDER_H

#include <QSlider>

class ClickMovableSlider : public QSlider
{
    Q_OBJECT
public:
    ClickMovableSlider(QWidget *parent = nullptr);

signals:
    void sigSeekTo(int position);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    void setPosition(int x);

private:
    bool m_bPressed{false};
    int  m_pressPosition{0};
};

#endif // CLICKMOVABLESLIDER_H
