#include "MainWidget.h"
#include "ui_mainwidget.h"
#include <QEvent>
#include <QScreen>
#include <QApplication>
#include <QStyle>
#include <QMouseEvent>
#include <QCursor>

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWidget)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setMouseTracking(true);

    ui->playlistWidget->setCursor(Qt::ArrowCursor);
    ui->videoWidget->setCursor(Qt::ArrowCursor);
    
    // Set initial window size and position
    resize(800, 600);
    
    // Center window on screen
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        const QRect availableGeometry = screen->availableGeometry();
        setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), availableGeometry));
    }
}

MainWidget::~MainWidget()
{
    delete ui;
}

void MainWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
        // Track the window state for proper button icon updates
        if (windowState() & Qt::WindowMaximized) {
            m_isMaximized = true;
        } else if (windowState() & Qt::WindowNoState) {
            m_isMaximized = false;
        }
    }
    
    QWidget::changeEvent(event);
}

void MainWidget::mousePressEvent(QMouseEvent *event)
{
    if (m_isMaximized) {
        QWidget::mousePressEvent(event);
        return;
    }
    
    // Store current mouse position
    m_dragPos = event->globalPos();
    
    // Check if user is trying to resize the window
    const QRect frameGeometry = this->frameGeometry();
    const int x = event->x();
    const int y = event->y();
    const int width = frameGeometry.width();
    const int height = frameGeometry.height();
    
    if (x <= m_borderWidth && y <= m_borderWidth) {
        m_dragEdge = MainWidget::EdgeType::TopLeft; // Top-left corner
    } else if (x <= m_borderWidth && y >= height - m_borderWidth) {
        m_dragEdge = MainWidget::EdgeType::BottomLeft; // Bottom-left corner
    } else if (x >= width - m_borderWidth && y <= m_borderWidth) {
        m_dragEdge = MainWidget::EdgeType::TopRight; // Top-right corner
    } else if (x >= width - m_borderWidth && y >= height - m_borderWidth) {
        m_dragEdge = MainWidget::EdgeType::BottomRight; // Bottom-right corner
    } else if (x <= m_borderWidth) {
        m_dragEdge = MainWidget::EdgeType::Left; // Left edge
    } else if (y <= m_borderWidth) {
        m_dragEdge = MainWidget::EdgeType::Top; // Top edge
    } else if (x >= width - m_borderWidth) {
        m_dragEdge = MainWidget::EdgeType::Right; // Right edge
    } else if (y >= height - m_borderWidth) {
        m_dragEdge = MainWidget::EdgeType::Bottom; // Bottom edge
    } else {
        m_dragEdge = MainWidget::EdgeType::None; // No edge
    }
    
    if (m_dragEdge != MainWidget::EdgeType::None) {
        m_resizing = true;
    }
    
    QWidget::mousePressEvent(event);
}

void MainWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isMaximized) {
        QWidget::mouseMoveEvent(event);
        return;
    }
    
    // Resize the window if we're in resize mode
    if (m_resizing) {
        const QPoint globalPos = event->globalPos();
        const QRect frameGeometry = this->frameGeometry();
        QRect newGeometry = frameGeometry;
        
        // Calculate the delta
        const int dx = globalPos.x() - m_dragPos.x();
        const int dy = globalPos.y() - m_dragPos.y();
        
        // Update drag position
        m_dragPos = globalPos;
        
        // Apply resize based on which edge is being dragged
        switch (m_dragEdge) {
        case MainWidget::EdgeType::Left: // Left edge
            newGeometry.setLeft(newGeometry.left() + dx);
            break;
        case MainWidget::EdgeType::Top: // Top edge
            newGeometry.setTop(newGeometry.top() + dy);
            break;
        case MainWidget::EdgeType::Right: // Right edge
            newGeometry.setRight(newGeometry.right() + dx);
            break;
        case MainWidget::EdgeType::Bottom: // Bottom edge
            newGeometry.setBottom(newGeometry.bottom() + dy);
            break;
        case MainWidget::EdgeType::TopLeft: // Top-left corner
            newGeometry.setTopLeft(newGeometry.topLeft() + QPoint(dx, dy));
            break;
        case MainWidget::EdgeType::TopRight: // Top-right corner
            newGeometry.setTopRight(newGeometry.topRight() + QPoint(dx, dy));
            break;
        case MainWidget::EdgeType::BottomLeft: // Bottom-left corner
            newGeometry.setBottomLeft(newGeometry.bottomLeft() + QPoint(dx, dy));
            break;
        case MainWidget::EdgeType::BottomRight: // Bottom-right corner
            newGeometry.setBottomRight(newGeometry.bottomRight() + QPoint(dx, dy));
            break;
        }
        
        // Apply the new geometry
        setGeometry(newGeometry);
    } else {
        // Update cursor shape based on mouse position
        const QRect frameGeometry = this->frameGeometry();
        const int x = event->x();
        const int y = event->y();
        const int width = frameGeometry.width();
        const int height = frameGeometry.height();
        
        // Determine cursor shape based on position
        if ((x <= m_borderWidth && y <= m_borderWidth) || 
            (x >= width - m_borderWidth && y >= height - m_borderWidth)) {
            setCursor(Qt::SizeFDiagCursor);
        } else if ((x <= m_borderWidth && y >= height - m_borderWidth) || 
                  (x >= width - m_borderWidth && y <= m_borderWidth)) {
            setCursor(Qt::SizeBDiagCursor);
        } else if (x <= m_borderWidth || x >= width - m_borderWidth) {
            setCursor(Qt::SizeHorCursor);
        } else if (y <= m_borderWidth || y >= height - m_borderWidth) {
            setCursor(Qt::SizeVerCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
    }
    
    QWidget::mouseMoveEvent(event);
}

void MainWidget::mouseReleaseEvent(QMouseEvent *event)
{
    m_resizing = false;
    m_dragEdge = MainWidget::EdgeType::None;
    
    QWidget::mouseReleaseEvent(event);
}
