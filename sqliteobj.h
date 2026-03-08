#ifndef SQLITEOBJ_H
#define SQLITEOBJ_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QVariantMap>
#include <QList>
#include <QDir>
#include <QDebug>

#include "header.h"

class SqliteObj : public QObject
{
    Q_OBJECT
private:
    explicit SqliteObj(QObject *parent = nullptr);
    ~SqliteObj() override;
    SqliteObj(const SqliteObj&) = delete;
    SqliteObj& operator=(const SqliteObj&) = delete;

public:
    // 单例获取接口（线程安全）
    static SqliteObj* getInstance();

private:
    // 初始化数据库（创建表）
    bool initDatabase();

public:

    // 获取置顶名称的分类信息
    CategoryInfo getCategoryInfo(const QString &name);

    // 获取所有分类信息列表
    QList<CategoryInfo> getCategoryList();

    // 添加分类
    bool appendCategory(const QString& name);

    // 删除分类(先删除分类下Mod联系)
    bool removeCategory(const int& catId);

    // 获取所有Mod信息列表
    QList<ModInfo> getModInfoList();

    // 获取指定路径和文件名的Mod文件信息
    ModInfo getModInfo(const QString &path, const QString &fileName);

    // 添加Mod信息项
    bool appendModInfo(const ModInfo &modInfo);

    // 移除Mod信息项
    bool removeModInfo(const int &modId);

    // 修改Mod自定义名称
    bool updateModCustomName(const int &modId, const QString &name);

    // 修改Mod相对路径
    bool updateModRelativePath(const int &modId, const QString &path);

    // 查询指定分类的Mod信息列表
    QList<ModInfo> getModInfoList(const int &catId);

    // 获取指定ModId的所有分类列表
    QList<CategoryInfo> getModCategorys(const int &modId);

    // 设置Mod分类
    bool setModCategory(const int &modId, const int &catId);

    // 移除Mod分类
    bool removeModCategory(const int &modId, const int &catId);

    // 更新分类名称
    bool updateCategoryName(const int &catId, const QString &name);

private:
    // 静态单例实例
    static SqliteObj* m_instance;
    // 数据库连接对象
    QSqlDatabase m_db;
    // 数据库文件路径
    const QString m_dbPath;
};

#endif // SQLITEOBJ_H
