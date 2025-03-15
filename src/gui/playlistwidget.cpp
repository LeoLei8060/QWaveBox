#include "playlistwidget.h"
#include "appcontext.h"
#include "playlistmodel.h"
#include "ui_playlistwidget.h"

PlaylistWidget::PlaylistWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PlaylistWidget)
{
    ui->setupUi(this);
    ui->tabWidget->setAttribute(Qt::WA_StyledBackground);

    setupTabWidget();
    setupDefaultList();
    setupComputerList();
}

PlaylistWidget::~PlaylistWidget()
{
    delete ui;
}

void PlaylistWidget::addFileToDefaultList(const QString &file)
{
    // file是文件的绝对路径
    ui->listView_def->addItem(file);
    AppContext::instance()->getAppData()->addPlayFileToDefAlbum(file);
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

void PlaylistWidget::setupDefaultList()
{
    m_defaultModel = new PlayListModel(this);
    ui->listView_def->setModel(m_defaultModel);

    auto album = AppContext::instance()->getAppData()->getDefaultAlbum();
    auto playFiles = album.getPlayfiles();
    for (auto file : playFiles) {
        ui->listView_def->addItem(file.filepath_);
    }

    connect(ui->listView_def,
            &PlayListView::sigFileDoubleClicked,
            this,
            &PlaylistWidget::sigOpenFile);
}

void PlaylistWidget::setupComputerList()
{
    connect(ui->listView, &FileListView::sigFileSelected, this, &PlaylistWidget::sigOpenFile);
}
