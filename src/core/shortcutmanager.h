#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#include <windows.h>
#include <QAbstractNativeEventFilter>
#include <QMap>
#include <QObject>

class ShortcutManager : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    static ShortcutManager *instance();

    // 注销所有快捷键
    void unregisterAllHotkeys();
    // 判断快捷键是否注册成功
    bool isHotkeyRegistered(const QString &shortcut, int id) const;

    // 注册快捷键
    bool registerHotkey(const QString &shortcut, int id);
    // 注销快捷键
    bool unregisterHotkey(int id);

    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;

signals:
    // 当快捷键被触发时发出信号
    void sigHotkeyTriggered(int id);

private:
    // 解析快捷键字符串
    QStringList parseShortcutString(const QString &shortcut) const;
    // 获取按键的虚拟键码
    UINT getVirtualKeyCode(const QString &keyStr) const;
    // 获取修饰键的标志
    UINT getModifiers(const QStringList &keys) const;

private:
    ShortcutManager(QObject *parent = nullptr);
    ~ShortcutManager();

    ShortcutManager(const ShortcutManager &) = delete;
    ShortcutManager &operator=(const ShortcutManager &) = delete;

    QMap<int, QPair<UINT, UINT>> m_registeredHotkeys; // id -> {vk, modifiers}
};

#endif // SHORTCUTMANAGER_H
