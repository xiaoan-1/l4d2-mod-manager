#include "sqliteobj.h"

#include <QMutex>

// 静态单例实例初始化
SqliteObj* SqliteObj::m_instance = nullptr;

SqliteObj::SqliteObj(QObject *parent)
    : QObject(parent)
    , m_dbPath(QDir::currentPath() + "/mod.db")
{
    // 初始化数据库连接（指定SQLite驱动，连接名唯一）
    if (QSqlDatabase::contains("ModManagerDB")) {
        QSqlDatabase::removeDatabase("ModManagerDB");
    }
    m_db = QSqlDatabase::addDatabase("QSQLITE", "ModManagerDB");
    m_db.setDatabaseName(m_dbPath);
    initDatabase();
}

SqliteObj::~SqliteObj()
{
    // 关闭数据库连接
    if (m_db.isOpen()) {
        m_db.close();
    }
}

/**
* @author   XiaoAn
* @brief    单例模式
* @date     2026-02-25
**/
SqliteObj *SqliteObj::getInstance()
{
    if (m_instance == nullptr) {
        static QMutex mutex;
        QMutexLocker locker(&mutex);
        if (m_instance == nullptr) {
            m_instance = new SqliteObj();
        }
    }
    return m_instance;
}

/**
* @author   XiaoAn
* @brief    初始化数据库
* @date     2026-02-25
**/
bool SqliteObj::initDatabase()
{
    // 打开数据库
    if (!m_db.open()) {
        qWarning() << "数据库打开失败：" << m_db.lastError().text();
        return false;
    }

    // 创建分类表
    QSqlQuery query(m_db);
    QString createTableSql = R"(
        CREATE TABLE IF NOT EXISTS categories (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT UNIQUE NOT NULL,
            sort INTEGER DEFAULT 0,
            create_time DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )";

    if (!query.exec(createTableSql)) {
        qWarning() << "分类表创建失败：" << query.lastError().text();
        return false;
    }

    // 创建Mod信息表
    createTableSql = R"(
        CREATE TABLE IF NOT EXISTS mod_mappings (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            relative_path TEXT NOT NULL,
            original_name TEXT UNIQUE NOT NULL,
            custom_name  DEFAULT '',
            create_time DATETIME DEFAULT CURRENT_TIMESTAMP,
            file_hash TEXT UNIQUE
        );
    )";
    if (!query.exec(createTableSql)) {
        qWarning() << "Mod信息表创建失败：" << query.lastError().text();
        return false;
    }

    // 创建Mod与分类的映射表
    createTableSql = R"(
        CREATE TABLE IF NOT EXISTS mod_category_relation (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            mod_id INTEGER NOT NULL,
            category_id INTEGER NOT NULL,
            create_time DATETIME DEFAULT CURRENT_TIMESTAMP,
            UNIQUE (mod_id, category_id),
            FOREIGN KEY (mod_id) REFERENCES mod_mappings(id) ON DELETE CASCADE,
            FOREIGN KEY (category_id) REFERENCES categories(id) ON DELETE CASCADE
        );
    )";
    if (!query.exec(createTableSql)) {
        qWarning() << "Mod-分类映射表创建失败：" << query.lastError().text();
        return false;
    }

    // 数据库表结构更新-添加hash值列
    query.exec("ALTER TABLE mod_mappings ADD COLUMN file_hash TEXT DEFAULT NULL;");
    if(!query.exec("CREATE UNIQUE INDEX IF NOT EXISTS idx_file_hash ON mod_mappings(file_hash)")){
        qWarning() << "Mod表添加唯一索引失败!";
    }
    return true;
}

/**
* @author   XiaoAn
* @brief    获取分类信息
* @date     2026-02-26
**/
CategoryInfo SqliteObj::getCategoryInfo(const QString &name)
{
    CategoryInfo category;

    QSqlQuery query(m_db);
    query.prepare("SELECT id, name, sort, create_time FROM categories WHERE name = :name;");
    query.bindValue(":name", name);

    if (!query.exec()) {
        qWarning() << "查询分类信息失败" << query.lastError().text();
        return category;
    }

    if (query.next()) {
        category.id = query.value(0).toInt();
        category.name = query.value(1).toString();
        category.sort = query.value(2).toInt();
        category.create_time = query.value(3).toDateTime();
    }
    return category;
}

/**
* @author   XiaoAn
* @brief    获取所有分类列表
* @date     2026-02-25
**/
QList<CategoryInfo> SqliteObj::getCategoryList()
{
    QList<CategoryInfo> categoryInfoList;

    QSqlQuery query(m_db);

    if (!query.exec("SELECT * FROM categories;")) {
        qWarning() << "查询分类信息失败" << query.lastError().text();
        return categoryInfoList;
    }

    while (query.next()) {
        CategoryInfo categoryInfo;
        categoryInfo.id = query.value(0).toInt();
        categoryInfo.name = query.value(1).toString();
        categoryInfo.create_time = query.value(2).toDateTime();
        categoryInfoList.append(categoryInfo);
    }

    return categoryInfoList;
}

/**
* @author   XiaoAn
* @brief    添加分类
* @date     2026-02-25
**/
bool SqliteObj::appendCategory(const QString &name)
{
    if (name.isEmpty()) return false;

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO categories (name) VALUES (:name)");
    query.bindValue(":name", name);
    if (!query.exec()) {
        qWarning() << "添加分类失败：" << query.lastError().text();
        return false;
    }
    return true;
}

/**
* @author   XiaoAn
* @brief    移除分类
* @date     2026-02-25
**/
bool SqliteObj::removeCategory(const int &catId)
{
    if(catId <= 0) return false;
    // 开启事务（确保原子性）
    m_db.transaction();

    // 删除Mod-分类关联（外键已配置CASCADE，可省略，但显式删除更安全）
    QSqlQuery delRelQuery(m_db);
    delRelQuery.prepare("DELETE FROM mod_category_relation WHERE category_id = :cat_id");
    delRelQuery.bindValue(":cat_id", catId);
    if (!delRelQuery.exec()) {
        m_db.rollback();
        qWarning() << "删除分类关联失败：" << delRelQuery.lastError().text();
        return false;
    }

    // 删除分类
    QSqlQuery delCatQuery(m_db);
    delCatQuery.prepare("DELETE FROM categories WHERE id = :cat_id");
    delCatQuery.bindValue(":cat_id", catId);
    if (!delCatQuery.exec()) {
        m_db.rollback();
        qWarning() << "删除分类失败：" << delCatQuery.lastError().text();
        return false;
    }

    // 提交事务
    return m_db.commit();
}

/**
* @author   XiaoAn
* @brief    获取Mod信息列表
* @date     2026-02-25
**/
QList<ModInfo> SqliteObj::getModInfoList(const QString &relativePath)
{
    QList<ModInfo> list;

    QSqlQuery query(m_db);

    QString sql = "SELECT * FROM mod_mappings";
    if(!relativePath.isEmpty()){
        sql += " where relative_path = :relative_path";
    }

    query.prepare(sql);
    query.bindValue(":relative_path", relativePath);

    if (!query.exec()) {
        qWarning() << "获取Mod列表失败：" << query.lastError().text();
        return list;
    }

    while (query.next()) {
        ModInfo info;
        info.id = query.value(0).toInt();
        info.relative_path = query.value(1).toString();
        info.original_name = query.value(2).toString();
        info.custom_name = query.value(3).toString();
        info.create_time = query.value(4).toDateTime();
        list.append(info);
    }

    return list;
}

/**
* @author   XiaoAn
* @brief    获取指定路径和文件里的Mod信息
* @date     2026-02-25
**/
ModInfo SqliteObj::getModInfo(const QString &path, const QString &fileName)
{
    ModInfo info;
    if (path.isEmpty() || fileName.isEmpty()) return info;

    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM mod_mappings WHERE relative_path=:path AND original_name=:file_name");
    query.bindValue(":path", path);
    query.bindValue(":file_name", fileName);
    if (!query.exec() ){
        qWarning() << "获取Mod信息失败：" << query.lastError().text();
        return info;
    }

    if(query.next()) {
        info.id = query.value(0).toInt();
        info.relative_path = query.value(1).toString();
        info.original_name = query.value(2).toString();
        info.custom_name = query.value(3).toString();
        info.create_time = query.value(4).toDateTime();
        info.file_hash = query.value(5).toString();
    }
    return info;
}

/**
* @author   XiaoAn
* @brief    查询指定hash的Mod信息
* @date     2026-03-31
**/
ModInfo SqliteObj::getModInfoByHash(const QString &hash)
{
    ModInfo info;
    if (hash.isEmpty()) return info;

    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM mod_mappings WHERE file_hash = :file_hash");
    query.bindValue(":file_hash", hash);
    if (!query.exec() ){
        qWarning() << "获取Mod信息失败：" << query.lastError().text();
        return info;
    }

    if(query.next()) {
        info.id = query.value(0).toInt();
        info.relative_path = query.value(1).toString();
        info.original_name = query.value(2).toString();
        info.custom_name = query.value(3).toString();
        info.create_time = query.value(4).toDateTime();
        info.file_hash = query.value(5).toString();
    }
    return info;
}

/**
* @author   XiaoAn
* @brief    添加Mod信息
* @date     2026-02-25
**/
bool SqliteObj::appendModInfo(const ModInfo &modInfo)
{
    if (modInfo.relative_path.isEmpty() || modInfo.original_name.isEmpty()) return false;

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO mod_mappings (relative_path, original_name, custom_name)
        VALUES (:relative_path, :original_name, :custom_name)
    )");
    query.bindValue(":relative_path", modInfo.relative_path);
    query.bindValue(":original_name", modInfo.original_name);
    query.bindValue(":custom_name", modInfo.custom_name);

    if (!query.exec()) {
        qWarning() << "添加Mod信息失败：" << modInfo.original_name << query.lastError().text();
        return false;
    }
    return true;
}

/**
* @author   XiaoAn
* @brief    移除Mod信息项
* @date     2026-02-25
**/
bool SqliteObj::removeModInfo(const int &modId)
{
    if (modId <= 0) return false;
    // 开启事务（确保原子性）
    m_db.transaction();

    // 删除Mod-分类关联（外键已配置CASCADE，可省略，但显式删除更安全）
    QSqlQuery delRelQuery(m_db);
    delRelQuery.prepare("DELETE FROM mod_category_relation WHERE mod_id = :mod_id");
    delRelQuery.bindValue(":mod_id", modId);
    if (!delRelQuery.exec()) {
        m_db.rollback();
        qWarning() << "删除Mod关联失败：" << delRelQuery.lastError().text();
        return false;
    }

    QSqlQuery delModQuery(m_db);
    delModQuery.prepare("DELETE FROM mod_mappings WHERE id = :mod_id");
    delModQuery.bindValue(":mod_id", modId);
    if (!delModQuery.exec()) {
        m_db.rollback();
        qWarning() << "删除Mod信息失败：" << delModQuery.lastError().text();
        return false;
    }
    // 返回是否真的删除了记录
    return m_db.commit();
}

/**
* @author   XiaoAn
* @brief    更新mod信息
* @date     2026-03-31
**/
bool SqliteObj::updateModInfo(const ModInfo &modInfo)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE mod_mappings set relative_path = :relative_path, original_name = :original_name,"
                  " custom_name = :name, create_time = :create_time, file_hash = :file_hash WHERE id = :mod_id");
    query.bindValue(":mod_id", modInfo.id);
    query.bindValue(":relative_path", modInfo.relative_path);
    query.bindValue(":original_name", modInfo.original_name);
    query.bindValue(":custom_name", modInfo.custom_name);
    query.bindValue(":create_time", modInfo.create_time);
    query.bindValue(":file_hash", modInfo.file_hash);

    if (!query.exec()) {
        qWarning() << "Mod信息修改失败：" << query.lastError().text();
        return false;
    }
    // 返回是否真的删除了记录
    return query.numRowsAffected() > 0;
}

/**
* @author   XiaoAn
* @brief    修改Mod自定义名称
* @date     2026-02-27
**/
bool SqliteObj::updateModCustomName(const int &modId, const QString &name)
{
    if(name.isEmpty()) return false;

    QSqlQuery query(m_db);
    query.prepare("UPDATE mod_mappings set custom_name = :name WHERE id = :mod_id");
    query.bindValue(":mod_id", modId);
    query.bindValue(":name", name);

    if (!query.exec()) {
        qWarning() << "Mod信息修改失败：" << query.lastError().text();
        return false;
    }
    // 返回是否真的删除了记录
    return query.numRowsAffected() > 0;
}

/**
* @author   XiaoAn
* @brief    修改Mod相对路径
* @date     2026-02-27
**/
bool SqliteObj::updateModRelativePath(const int &modId, const QString &path)
{
    if(path.isEmpty()) return false;

    QSqlQuery query(m_db);
    query.prepare("UPDATE mod_mappings set relative_path = :path WHERE id = :mod_id");
    query.bindValue(":mod_id", modId);
    query.bindValue(":path", path);

    if (!query.exec()) {
        qWarning() << "Mod信息修改失败：" << query.lastError().text();
        return false;
    }
    // 返回是否真的删除了记录
    return query.numRowsAffected() > 0;
}

/**
* @author   XiaoAn
* @brief    查询Mod信息
* @date     2026-02-26
**/
QList<ModInfo> SqliteObj::getModInfoList(const int &catId)
{
    QList<ModInfo> modList;

    // 校验分类ID合法性
    if (catId <= 0) {
        qWarning() << "分类ID不合法：" << catId;
        return modList;
    }

    // 多表联查：mod + mod_category_relation
    QSqlQuery query(m_db);
    QString selectSql = R"(
        SELECT m.id, m.relative_path, m.original_name, m.custom_name, m.create_time FROM mod_mappings m
        INNER JOIN mod_category_relation r ON m.id = r.mod_id
        WHERE r.category_id = :cat_id
        ORDER BY m.create_time DESC
    )";

    query.prepare(selectSql);
    query.bindValue(":cat_id", catId); // 绑定分类ID参数，防止SQL注入

    if (!query.exec()) {
        qCritical() << "查询分类Mod失败：" << query.lastError().text();
        return modList;
    }

    // 遍历查询结果，封装为ModInfo列表
    while (query.next()) {
        ModInfo mod;
        mod.id = query.value(0).toInt();
        mod.relative_path = query.value(1).toString();
        mod.original_name = query.value(2).toString();
        mod.custom_name = query.value(3).toString();
        mod.create_time = query.value(4).toDateTime();

        modList.append(mod);
    }
    return modList;
}


/**
* @author   XiaoAn
* @brief    获取指定Mod的分类列表
* @date     2026-02-25
**/
QList<CategoryInfo> SqliteObj::getModCategorys(const int &modId)
{
    QList<CategoryInfo> list;
    if (modId <= 0) return list;

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT c.id, c.name, c.sort, c.create_time
        FROM categories c
        JOIN mod_category_relation r ON c.id = r.category_id
        WHERE r.mod_id = :mod_id
        ORDER BY c.sort ASC
    )");
    query.bindValue(":mod_id", modId);
    if (query.exec()) {
        while (query.next()) {
            CategoryInfo info;
            info.id = query.value(0).toInt();
            info.name = query.value(1).toString();
            info.sort = query.value(2).toInt();
            info.create_time = query.value(3).toDateTime();
            list.append(info);
        }
    } else {
        qWarning() << "获取Mod分类失败：" << query.lastError().text();
    }
    return list;
}



/**
* @author   XiaoAn
* @brief    设置指定Mod的分类
* @date     2026-02-25
**/
bool SqliteObj::setModCategory(const int &modId, const int &catId)
{
    // 入参校验
    if (modId <= 0 || catId <= 0) {
        qWarning() << "设置Mod分类失败：参数无效或数据库未打开";
        return false;
    }

    // 1. 先检查分类是否存在
    QSqlQuery checkCatQuery(m_db);
    checkCatQuery.prepare("SELECT id FROM categories WHERE id = :cat_id");
    checkCatQuery.bindValue(":cat_id", catId);
    if (!checkCatQuery.exec() || !checkCatQuery.next()) {
        qWarning() << "设置Mod分类失败：分类ID不存在";
        return false;
    }

    // 2. 检查Mod是否存在
    QSqlQuery checkModQuery(m_db);
    checkModQuery.prepare("SELECT id FROM mod_mappings WHERE id = :mod_id");
    checkModQuery.bindValue(":mod_id", modId);
    if (!checkModQuery.exec() || !checkModQuery.next()) {
        qWarning() << "设置Mod分类失败：Mod ID不存在";
        return false;
    }

    // 3. 插入关联记录（INSERT OR IGNORE 避免重复关联）
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT OR IGNORE INTO mod_category_relation (mod_id, category_id)
        VALUES (:mod_id, :category_id)
    )");
    query.bindValue(":mod_id", modId);
    query.bindValue(":category_id", catId);

    if (!query.exec()) {
        qWarning() << "设置Mod分类失败：" << query.lastError().text();
        return false;
    }

    // 检查是否真的插入了记录（0表示已存在，也视为成功）
    return true;
}

/**
* @author   XiaoAn
* @brief    移除关联
* @date     2026-02-25
**/
bool SqliteObj::removeModCategory(const int &modId, const int &catId)
{
    // 入参校验
    if (modId <= 0 || catId <= 0) {
        qWarning() << "移除Mod分类失败：参数无效或数据库未打开";
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM mod_category_relation WHERE mod_id = :mod_id AND category_id = :category_id");
    query.bindValue(":mod_id", modId);
    query.bindValue(":category_id", catId);

    if (!query.exec()) {
        qWarning() << "移除Mod分类失败：" << query.lastError().text();
        return false;
    }

    // 返回是否真的删除了记录（影响行数>0）
    return query.numRowsAffected() > 0;
}

/**
* @author   XiaoAn
* @brief    更新分类名称
* @date     2026-03-08
**/
bool SqliteObj::updateCategoryName(const int &catId, const QString &name)
{
    if(name.isEmpty()) return false;

    QSqlQuery query(m_db);
    query.prepare("UPDATE categories set name = :name WHERE id = :cat_id");
    query.bindValue(":cat_id", catId);
    query.bindValue(":name", name);

    if (!query.exec()) {
        qWarning() << "分类名称修改失败：" << query.lastError().text();
        return false;
    }
    // 返回是否真的修改了记录
    return query.numRowsAffected() > 0;
}
