#include "titlebar.h"
#include "ui_titlebar.h"
#include "fontmanager.h"
#include <QApplication>
#include <QFontDatabase>
#include <QDebug>

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TitleBar)
{
    ui->setupUi(this);

    // initialize iconfont
    m_iconFont = FontManager::instance()->fontAt(FontManager::IconFont);
    m_iconFont.setPixelSize(16);
    
    setupButtons();
    
    // Connect button signals
    connect(ui->pushButton, &QPushButton::clicked, this, &TitleBar::onPinButtonClicked);
    connect(ui->pushButton_2, &QPushButton::clicked, this, &TitleBar::onMinimizeButtonClicked);
    connect(ui->pushButton_3, &QPushButton::clicked, this, &TitleBar::onMaximizeButtonClicked);
    connect(ui->pushButton_4, &QPushButton::clicked, this, &TitleBar::onFullscreenButtonClicked);
    connect(ui->pushButton_5, &QPushButton::clicked, this, &TitleBar::onCloseButtonClicked);
}

TitleBar::~TitleBar()
{
    delete ui;
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // Get the position of all buttons
        QRect pinBtnRect = ui->pushButton->geometry();
        QRect minBtnRect = ui->pushButton_2->geometry();
        QRect maxBtnRect = ui->pushButton_3->geometry();
        QRect fullscreenBtnRect = ui->pushButton_4->geometry();
        QRect closeBtnRect = ui->pushButton_5->geometry();
        
        // Check if the mouse is in the non-button area
        if (!pinBtnRect.contains(event->pos()) && 
            !minBtnRect.contains(event->pos()) && 
            !maxBtnRect.contains(event->pos()) && 
            !fullscreenBtnRect.contains(event->pos()) && 
            !closeBtnRect.contains(event->pos())) {
            m_isPressed = true;
            m_dragPosition = event->globalPos() - this->parentWidget()->frameGeometry().topLeft();
            event->accept();
        }
    }
    
    QWidget::mousePressEvent(event);
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton && m_isPressed) {
        QWidget *parent = this->parentWidget();
        if (parent) {
            parent->move(event->globalPos() - m_dragPosition);
            event->accept();
        }
    }
    
    QWidget::mouseMoveEvent(event);
}

void TitleBar::mouseReleaseEvent(QMouseEvent *event)
{
    m_isPressed = false;
    QWidget::mouseReleaseEvent(event);
}

void TitleBar::setupButtons()
{
    // Set icon font for all buttons
    ui->pushButton->setFont(m_iconFont);
    ui->pushButton_2->setFont(m_iconFont);
    ui->pushButton_3->setFont(m_iconFont);
    ui->pushButton_5->setFont(m_iconFont);
    m_iconFont.setPixelSize(12);
    ui->pushButton_4->setFont(m_iconFont);
    
    // Set initial icon text
    ui->pushButton->setText(QChar(0xe60b));   // Pin icon
    ui->pushButton_2->setText(QChar(0xe612)); // Minimize icon
    ui->pushButton_3->setText(QChar(0xe799)); // Maximize icon
    ui->pushButton_4->setText(QChar(0xe607)); // Fullscreen icon
    ui->pushButton_5->setText(QChar(0xe60f)); // Close icon
}

void TitleBar::updateButtonStates()
{
    // Update pin button icon based on current state
    ui->pushButton->setText(QChar(m_isPinned ? 0xe60c : 0xe60b));
    
    // Update maximize button icon based on current state
    ui->pushButton_3->setText(QChar(m_isMaximized ? 0xe60d : 0xe799));
    
    // Update fullscreen button icon based on current state
    ui->pushButton_4->setText(QChar(m_isFullscreen ? 0xe608 : 0xe607));
}

void TitleBar::onPinButtonClicked()
{
    m_isPinned = !m_isPinned;
    updateButtonStates();
    
    QWidget *mainWindow = this->parentWidget();
    if (mainWindow) {
        Qt::WindowFlags flags = mainWindow->windowFlags();
        if (m_isPinned) {
            flags |= Qt::WindowStaysOnTopHint;
        } else {
            flags &= ~Qt::WindowStaysOnTopHint;
        }
        mainWindow->setWindowFlags(flags);
        mainWindow->show(); // Need to show again after changing flags
    }
}

void TitleBar::onMinimizeButtonClicked()
{
    QWidget *mainWindow = this->parentWidget();
    if (mainWindow) {
        mainWindow->showMinimized();
    }
}

void TitleBar::onMaximizeButtonClicked()
{
    QWidget *mainWindow = this->parentWidget();
    if (mainWindow) {
        if (m_isMaximized) {
            mainWindow->showNormal();
        } else {
            mainWindow->showMaximized();
        }
        m_isMaximized = !m_isMaximized;
        updateButtonStates();
    }
}

void TitleBar::onFullscreenButtonClicked()
{
    QWidget *mainWindow = this->parentWidget();
    if (mainWindow) {
        if (m_isFullscreen) {
            // Exit fullscreen
            mainWindow->setWindowState(mainWindow->windowState() & ~Qt::WindowFullScreen);
        } else {
            // Enter fullscreen
            mainWindow->setWindowState(mainWindow->windowState() | Qt::WindowFullScreen);
        }
        m_isFullscreen = !m_isFullscreen;
        updateButtonStates();
    }
}

void TitleBar::onCloseButtonClicked()
{
    QWidget *mainWindow = this->parentWidget();
    if (mainWindow) {
        mainWindow->close();
    }
}
