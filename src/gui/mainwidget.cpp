#include "MainWidget.h"
#include "ui_mainwidget.h"
#include <QEvent>
#include <QScreen>
#include <QApplication>
#include <QStyle>

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWidget)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    
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
