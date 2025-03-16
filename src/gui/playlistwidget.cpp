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
    if (ui->listView_def->addItem(file))
        AppContext::instance()->getAppData()->addPlayFileToDefAlbum(file);
}

void PlaylistWidget::playSelected()
{
    int index = ui->tabWidget->currentIndex();
    if (0 == index) {
        // 默认专辑
        if (auto path = ui->listView_def->getSelectedPath(); !path.isEmpty())
            emit sigOpenFile(path);
    } else if (1 == index) {
        // 此电脑专辑
        if (auto path = ui->listView->getSelectedPath(); !path.isEmpty())
            emit sigOpenFile(path);
    } else {
        // 新增自定义专辑
        // TODO: 待处理
    }
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
