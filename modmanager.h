#ifndef MODMANAGER_H
#define MODMANAGER_H

#include <QObject>

#include "sqliteobj.h"

class ModManager : public QObject
{
    Q_OBJECT
private:
    explicit ModManager(QObject *parent = nullptr);
    ~ModManager();
    // 禁止拷贝
    ModManager(const ModManager&) = delete;
    ModManager& operator=(const ModManager&) = delete;

public:
    static ModManager* getInstance(QObject *parent = nullptr);

    static QString getFileSizeWithUnit(const quint64 &fileSize);
public:    
    QString gamePath() const;

    // 设置游戏根路径
    bool setGamePath(const QString& path);

    // 扫描相对路径下的Mod文件信息
    QList<ModInfo> scanDirModInfo(const QString &relativePath);

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
};

#endif // MODMANAGER_H
