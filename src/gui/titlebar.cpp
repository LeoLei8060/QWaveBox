#include "titlebar.h"
#include "fontmanager.h"
#include "ui_titlebar.h"
#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QFontDatabase>
#include <QStyle>

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TitleBar)
{
    ui->setupUi(this);
    setCursor(Qt::ArrowCursor);

    // initialize iconfont
    m_iconFont = FontManager::instance()->fontAt(FontManager::IconFont);
    m_iconFont.setPixelSize(16);

    setupButtons();
    setupGlobalShortcuts();

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

    delete m_openFileShortcut;
    delete m_openFolderShortcut;
    delete m_closeToTrayShortcut;
    delete m_optionsShortcut;
    delete m_aboutShortcut;
    delete m_quitShortcut;
}

void TitleBar::setMenu(QMenu *menu)
{
    // 设置QToolButton的菜单
    ui->toolButton->setMenu(menu);

    connect(ui->toolButton, &QToolButton::clicked, this, [this, menu]() {
        menu->exec(mapToGlobal(ui->toolButton->pos()) + QPoint(0, 30));
    });
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
        if (!pinBtnRect.contains(event->pos()) && !minBtnRect.contains(event->pos())
            && !maxBtnRect.contains(event->pos()) && !fullscreenBtnRect.contains(event->pos())
            && !closeBtnRect.contains(event->pos())) {
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

void TitleBar::setupGlobalShortcuts()
{
    // 创建全局快捷键
    m_openFileShortcut = new QShortcut(QKeySequence(Qt::Key_F3), this);
    m_openFolderShortcut = new QShortcut(QKeySequence(Qt::Key_F2), this);
    m_closeToTrayShortcut = new QShortcut(QKeySequence(Qt::Key_F4), this);
    m_optionsShortcut = new QShortcut(QKeySequence(Qt::Key_F5), this);
    m_aboutShortcut = new QShortcut(QKeySequence(Qt::Key_F1), this);
    m_quitShortcut = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_F4), this);

    // 连接快捷键信号和槽
    connect(m_openFileShortcut, &QShortcut::activated, this, &TitleBar::openFileRequested);
    connect(m_openFolderShortcut, &QShortcut::activated, this, &TitleBar::openFolderRequested);
    connect(m_closeToTrayShortcut, &QShortcut::activated, this, &TitleBar::closeToTrayRequested);
    connect(m_optionsShortcut, &QShortcut::activated, this, &TitleBar::optionsRequested);
    connect(m_aboutShortcut, &QShortcut::activated, this, &TitleBar::aboutRequested);
    connect(m_quitShortcut, &QShortcut::activated, this, &TitleBar::quitApplicationRequested);
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
    qDebug() << __FUNCTION__;
    // 发送关闭到托盘信号
    emit closeToTrayRequested();
}
