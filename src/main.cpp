#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    
    // 设置应用信息
    QApplication::setApplicationName("VideoPlayer");
    QApplication::setApplicationVersion("1.0");
    QApplication::setOrganizationName("VideoPlayerOrg");
    
    // 命令行参数解析
    QCommandLineParser parser;
    parser.setApplicationDescription("A simple video player based on Qt and FFmpeg");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "The file to open");
    parser.process(a);
    
    MainWindow w;
    w.show();
    
    // 如果命令行中指定了文件，则打开该文件
    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) {
        const QString fileName = args.first();
        if (QFileInfo::exists(fileName)) {
            w.openFile(fileName);
        }
    }
    
    return a.exec();
}
