#include "playlistwidget.h"
#include "ui_playlistwidget.h"

PlaylistWidget::PlaylistWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PlaylistWidget)
{
    ui->setupUi(this);
    ui->tabWidget->setAttribute(Qt::WA_StyledBackground);

    setupTabWidget();
    setupComputerList();
}

PlaylistWidget::~PlaylistWidget()
{
    delete ui;
}

void PlaylistWidget::setupTabWidget()
{
    ui->tabWidget->setTabText(0, "默认专辑");
    ui->tabWidget->setTabText(1, "此电脑");

    // 创建“新建专辑”按钮并绑定
    m_addTabBtn = new QPushButton("+ 新建专辑", this);
    m_addTabBtn->setObjectName("AddPlaylistBtn");
    ui->tabWidget->setCornerWidget(m_addTabBtn, Qt::Corner::TopLeftCorner);
}

void PlaylistWidget::setupComputerList()
{
    connect(ui->listView, &FileListView::sigFileSelected, this, &PlaylistWidget::sigOpenFile);
}
