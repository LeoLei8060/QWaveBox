#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>

class Application : public QApplication
{
    Q_OBJECT
public:
    Application(int &argc, char **argv, const QString &appKey);
    ~Application() override;

    // NOTE: 初始化应用程序（业务）
    bool initialize();

    void setIconPath(const QString &path) { m_iconPath = path; }
    void setIconFontPath(const QString &path) { m_iconfontPath = path; }
    void setStylePath(const QString &path) { m_stylePath = path; }

private:
    bool initializeIcon();
    bool initializeIconFont();
    bool initializeStyle();
    bool initializeHotkey();

private:
    QString m_appKey;
    QString m_iconPath;
    QString m_iconfontPath;
    QString m_stylePath;
};

#endif // APPLICATION_H
