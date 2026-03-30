#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <QObject>

#include "sqliteobj.h"

class GameManager : public QObject
{
    Q_OBJECT
private:
    explicit GameManager(QObject *parent = nullptr);
    ~GameManager();
    // 禁止拷贝
    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;

public:
    static GameManager* getInstance(QObject *parent = nullptr);

    static QString getFileSizeWithUnit(const quint64 &fileSize);
public:
    QString gamePath() const { return m_gamePath; };

    QString gameParam() const { return m_gameParam; };

    // 设置游戏根路径
    bool setGamePath(const QString& path);

    // 设置游戏启动参数
    void setGameParam(const QString& gameParam);

    // 扫描相对路径下的Mod文件信息
    QList<ModInfo> scanDirModInfo(const QString &relativePath);

    // 拷贝文件到游戏路径下
    bool copyDirectory(const QString &srcDir, const QString &dstDir);

private:
    // 本地文件和数据库同步信息
    void syncModInfo();

public:
    static const QString ModLocalDir;
    static const QString WorkshopDir;
    static const QString ModTrashDir;

private:
    // 游戏根路径
    QString m_gamePath;

    // 游戏启动参数
    QString m_gameParam;
};

#endif // GAMEMANAGER_H
