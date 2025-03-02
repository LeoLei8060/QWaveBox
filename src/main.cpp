#include "application.h"
#include "mainWidget.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFileInfo>
#include <QIcon>

int main(int argc, char *argv[])
{
    Application a(argc, argv, "QWaveBox");

    a.setIconFontPath(":/resources/iconFont/iconfont.ttf");
    a.setIconPath(":/resources/wavebox.ico");
    a.setStylePath("://resources/black.qss");

    if (!a.initialize())
        return 0;

    // 命令行参数解析
    QCommandLineParser parser;
    parser.setApplicationDescription("A simple video player based on Qt and FFmpeg");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "The file to open");
    parser.process(a);

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
