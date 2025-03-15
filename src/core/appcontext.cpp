#include "appcontext.h"
#include <string>
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QTextCodec>

#define CONFIG_PATH "./config.json"

bool bufferToFile(const QString &str, const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    f.write(str.toLocal8Bit().data());
    f.close();
    return true;
}

AppContext *AppContext::instance()
{
    static AppContext instance;
    return &instance;
}

AppContext::AppContext(QObject *parent)
    : QObject(parent)
    , m_appData(std::make_shared<AppData>())
{
    QFileInfo fileInfo(CONFIG_PATH);
    if (fileInfo.isFile()) {
        auto [ret, json] = reflexjson::json_to_obj(CONFIG_PATH, m_appData, true);
        if (ret)
            qDebug() << "read config successfull." << m_appData->getDefaultAlbumName();
        else
            qWarning() << "read config failed.";
    }
}

AppContext::~AppContext()
{
    auto json = reflexjson::obj_to_json(m_appData);
    bool ret = bufferToFile(QString::fromStdString(json), CONFIG_PATH);
    if (!ret)
        qWarning() << "config save failed.";
}
