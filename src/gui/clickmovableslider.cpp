#include "clickmovableslider.h"
#include <QDebug>
#include <QMouseEvent>

ClickMovableSlider::ClickMovableSlider(QWidget *parent)
    : QSlider(parent)
{
    setMouseTracking(true);
}

void ClickMovableSlider::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_bPressed = true;
        m_pressPosition = event->pos().x();
        setPosition(m_pressPosition);
    }
}

void ClickMovableSlider::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_bPressed) {
        if (event->pos().x() != m_pressPosition)
            setPosition(event->pos().x());
        m_pressPosition = 0;
        m_bPressed = false;
    }
}

void ClickMovableSlider::mouseMoveEvent(QMouseEvent *event)
{
    if (m_bPressed) {
        setPosition(event->pos().x());
    }
}

void ClickMovableSlider::setPosition(int x)
{
    int duration = maximum() - minimum();
    int position = minimum() + ((double) x / width()) * duration;
    if (abs(position - sliderPosition()) > 1) {
        setValue(position);
        emit sigSeekTo(position);
    }
}
