#include "application.h"
#include "fontmanager.h"

#include <QDebug>
#include <QFile>
#include <QIcon>

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
