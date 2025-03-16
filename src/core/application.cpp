#include "application.h"
#include "constants.h"
#include "fontmanager.h"
#include "shortcutmanager.h"

#include <QDebug>
#include <QFile>
#include <QIcon>
#include <QMap>

Application::Application(int &argc, char **argv, const QString &appKey)
    : QApplication(argc, argv)
    , m_appKey(appKey)
{
    // 设置应用信息
    QApplication::setOrganizationName("LeoLei8060");
    QApplication::setApplicationName(m_appKey);
    QApplication::setApplicationVersion("1.0.0");
}

Application::~Application() {}

bool Application::initialize()
{
    // initializeHotkey();
    if (initializeIcon() && initializeIconFont() && initializeStyle())
        return true;
    return false;
}

bool Application::initializeIcon()
{
    if (m_iconPath.isEmpty())
        return true; // 允许没有图标
    QIcon appIcon(m_iconPath);
    if (appIcon.isNull())
        return false;
    QApplication::setWindowIcon(appIcon);
    return true;
}

bool Application::initializeIconFont()
{
    if (!FontManager::instance()->addThirdpartyFont(m_iconfontPath, FontManager::IconFont)) {
        qWarning() << "Failed to load icon font.";
        return false;
    }
    return true;
}

bool Application::initializeStyle()
{
    QFile   file(m_stylePath);
    QString stylesheets = "";
    if (file.open(QFile::ReadOnly)) {
        stylesheets = QLatin1String(file.readAll());
        this->setStyleSheet(stylesheets);
        file.close();
        return true;
    }
    return false;
}

bool Application::initializeHotkey()
{
    // NOTE: ShortcutManager管理的是全局快捷键，这里暂时用不到
    static QMap<int, QString> hotkeys = {{K_OpenFile, "F3"},
                                         {K_OpenFolder, "F2"},
                                         {K_Close, "F4"},
                                         {K_Options, "F5"},
                                         {K_About, "F1"},
                                         {K_Quit, "Ctrl+F4"},
                                         {K_PlaySelected, "Enter"},
                                         {K_PausePlay, "Space"},
                                         {K_NextPlay, "Right"},
                                         {K_PrevPlay, "Left"}};
    for (auto it = hotkeys.begin(); it != hotkeys.end(); ++it) {
        ShortcutManager::instance()->registerHotkey(it.value(), it.key());
    }
    return true;
}
