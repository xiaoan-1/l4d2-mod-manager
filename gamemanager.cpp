#include "gamemanager.h"

#include <QDir>
#include <QFile>
#include <QMutex>
#include <QSettings>
#include <QFileInfoList>

#include "sqliteobj.h"

const QString GameManager::ModLocalDir = "/left4dead2/addons";
const QString GameManager::WorkshopDir = "/left4dead2/addons/workshop";
const QString GameManager::ModTrashDir = "/left4dead2/addons/trash";

GameManager::GameManager(QObject *parent)
    : QObject(parent)
{
    // 读取游戏路径配置
    QSettings setting(QDir::currentPath() + "/config.ini", QSettings::IniFormat);
    m_gamePath = setting.value("GamePath").toString();
    m_gameParam = setting.value("GameParam").toString();

    syncModInfo();
}

GameManager::~GameManager()
{

}

/**
* @author   XiaoAn
* @brief    单例对象
* @date     2026-02-25
**/
GameManager *GameManager::getInstance(QObject *parent)
{
    static GameManager* instance = nullptr;
    static QMutex mutex;
    if (!instance) {
        QMutexLocker locker(&mutex);
        if (!instance) {
            instance = new GameManager(parent);
        }
    }
    return instance;
}

/**
* @author   XiaoAn
* @brief    文件大小单位
* @date     2026-02-26
**/
QString GameManager::getFileSizeWithUnit(const quint64 &fileSize)
{
    const int base = 1024;
    const QStringList units = {"B", "KB", "MB", "GB", "TB", "PB"};

    // 对数运算获取单位索引
    int unitLevel = static_cast<int>(std::log(fileSize) / std::log(base));
    // 防止超出单位列表范围
    unitLevel = qBound(0, unitLevel, units.size() - 1);

    double sizeWithUnit = static_cast<double>(fileSize) / std::pow(base, unitLevel);
    QString sizeText = QString("%1 %2").arg(sizeWithUnit, 0, 'f', 2).arg(units.at(unitLevel));
    return sizeText;
}

/**
* @author   XiaoAn
* @brief    设置游戏根目录
* @date     2026-02-25
**/
bool GameManager::setGamePath(const QString &path)
{
    // 验证目录
    QDir dir(path + ModLocalDir);
    if(!dir.exists()){
        return false;
    }

    // 添加垃圾桶目录
    dir.setPath(path + ModTrashDir);
    if (!dir.exists()) {
        dir.mkpath(path + ModTrashDir);
    }

    // 记录路径
    QSettings setting(QDir::currentPath() + "/config.ini", QSettings::IniFormat);
    setting.setValue("GamePath", m_gamePath);

    syncModInfo();

    return true;
}

/**
* @author   XiaoAn
* @brief    设置游戏启动参数
* @date     2026-03-02
**/
void GameManager::setGameParam(const QString &gameParam)
{
    // 第三方启动附带steam参数，通过steam启动
    m_gameParam = gameParam;
    QSettings setting(QDir::currentPath() + "/config.ini", QSettings::IniFormat);
    setting.setValue("GameParam", m_gameParam);
}

/**
* @author   XiaoAn
* @brief    与数据库同步Mod信息
* @date     2026-02-25
**/
void GameManager::syncModInfo()
{
    // 遍历数据库所有mod信息，丢弃不存在的文件mod信息
    QList<ModInfo> modInfoList = SqliteObj::getInstance()->getModInfoList();
    foreach (const ModInfo &modInfo, modInfoList) {
        // 校验本地路径是否存在该mod
        QFile file(m_gamePath + modInfo.relative_path + "/" + modInfo.original_name + ".vpk");
        // 如果不存在则在数据库删除该mod信息
        if(!file.exists()){
            SqliteObj::getInstance()->removeModInfo(modInfo.id);
        }
    }
}


/**
* @author   XiaoAn
* @brief    扫描指定路径下的mod
* @date     2026-02-25
**/
QList<ModInfo> GameManager::scanDirModInfo(const QString &relativePath)
{
    QDir dir(m_gamePath + relativePath);
    QStringList filter;
    filter << "*.vpk";
    QFileInfoList fileInfoList = dir.entryInfoList(filter, QDir::Files);

    QList<ModInfo> modInfoList;
    foreach (const QFileInfo &fileInfo, fileInfoList) {
        ModInfo modInfo = SqliteObj::getInstance()->getModInfo(relativePath, fileInfo.baseName());
        if(modInfo.id == -1){
            // 如果没有该文件的记录则添加记录
            modInfo.relative_path = relativePath;
            modInfo.original_name = fileInfo.baseName();
            modInfo.custom_name = "";
            SqliteObj::getInstance()->appendModInfo(modInfo);
        }
        modInfoList.append(modInfo);
    }
    return modInfoList;
}

/**
* @author   XiaoAn
* @brief    安装补丁
* @date     2026-03-30
**/
bool GameManager::copyDirectory(const QString &srcPath, const QString &dstDir)
{
    bool success = true;
    QDir srcDir(srcPath);

    // 遍历所有条目
    QFileInfoList entries = srcDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (const QFileInfo &entry , entries) {
        QString srcPath = entry.absoluteFilePath();
        QString dstPath = dstDir + "/" + entry.fileName();

        if (entry.isDir()) {
            QDir().mkdir(dstPath);
            // 递归拷贝子目录
            if (!copyDirectory(srcPath, dstPath)) {
                success = false;
            }
        } else {

            // 拷贝文件（覆盖）
            if (QFile::exists(dstPath)) {
                QFile::remove(dstPath);
            }

            if (!QFile::copy(srcPath, dstPath)) {
                qDebug() << "拷贝失败:" << srcPath;
                success = false;
            }
        }
    }
    return success;
}

