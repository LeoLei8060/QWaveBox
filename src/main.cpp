#include "fontmanager.h"
#include "mainWidget.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFileInfo>
#include <QIcon>

bool initializeFonts()
{
    if (!FontManager::instance()->addThirdpartyFont(":/resources/iconFont/iconfont.ttf",
                                                    FontManager::IconFont)) {
        qWarning() << "Failed to load icon font.";
        return false;
    }
    return true;
}

bool initializeStyle(QApplication *app)
{
    QFile   file("://resources/black.qss");
    QString stylesheets = "";
    if (file.open(QFile::ReadOnly)) {
        stylesheets = QLatin1String(file.readAll());
        app->setStyleSheet(stylesheets);
        file.close();
        return true;
    }
    return false;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 设置应用信息
    QApplication::setApplicationName("QWaveBox");
    QApplication::setApplicationVersion("1.0");
    QApplication::setOrganizationName("QWaveBoxOrg");

    // 设置应用程序图标
    QIcon appIcon(":/resources/wavebox.ico");
    QApplication::setWindowIcon(appIcon);

    // 命令行参数解析
    QCommandLineParser parser;
    parser.setApplicationDescription("A simple video player based on Qt and FFmpeg");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "The file to open");
    parser.process(a);

    if (!initializeFonts())
        return 0;
    if (!initializeStyle(&a))
        return 0;

    MainWidget w;
    w.show();

    // 如果命令行中指定了文件，则打开该文件
    // const QStringList args = parser.positionalArguments();
    // if (!args.isEmpty()) {
    //     const QString fileName = args.first();
    //     if (QFileInfo::exists(fileName)) {
    //         w.openFile(fileName);
    //     }
    // }

    return a.exec();
}
