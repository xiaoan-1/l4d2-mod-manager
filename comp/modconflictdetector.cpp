#include "modconflictdetector.h"

#include <QFile>
#include <QFileInfo>
#include <QDebug>

ModConflictDetector::ModConflictDetector(QObject *parent)
    : QObject{parent}
{}

ModConflictDetector::~ModConflictDetector()
{}


/**
* @author   XiaoAn
* @brief    解析vpk获取文件条目信息
* @date     2026-03-11
**/
QList<ModFileInfo> ModConflictDetector::parseVPK(const QString &vpkPath)
{
    QList<ModFileInfo> files;
    QFileInfo fileInfo(vpkPath);
    if(!fileInfo.exists()){
        return files;
    }

    if (fileInfo.suffix() != "vpk") {
        qWarning() << "not vpk file! " << vpkPath;
        return files;
    }

    QFile file(vpkPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open file:" << vpkPath;
        return files;
    }

    QDataStream in(&file);
    // 使用小端字节序
    in.setByteOrder(QDataStream::LittleEndian);

    // ============================读取VPK文件头结构=============
    quint32 signature;
    quint32 version;
    quint32 treeSize;
    in >> signature >> version >> treeSize;
    // 验证签名 (0x55AA1234)
    if (signature != 0x55AA1234) {
        qWarning() << "Invalid VPK signature:" << vpkPath;
        file.close();
        return files;
    }
    // 如果是版本2，需要读取额外字段
    if (version == 2) {
        quint32 fileDataSectionSize;
        quint32 archiveMD5SectionSize;
        quint32 otherMD5SectionSize;
        quint32 signatureSectionSize;
        in >> fileDataSectionSize >> archiveMD5SectionSize >> otherMD5SectionSize >> signatureSectionSize;
    }


    // ==========================解析目录树======================
    qint64 treeStartPos = file.pos();
    qint64 treeEndPos = treeStartPos + treeSize;

    char c;
    QByteArray extension, path, filename;
    // 按扩展名->路径->文件名 的三层结构遍历
    while (file.pos() < treeEndPos) {
        // 1. 读取扩展名
        extension.clear();
        while (file.getChar(&c) && c != '\0') {
            extension.append(c);
        }
        if (extension.isEmpty()) break; // 结束标记

        // 2. 读取路径
        path.clear();
        while (file.getChar(&c) && c != '\0') {
            path.append(c);
        }

        // 3. 读取文件名
        filename.clear();
        while (file.getChar(&c) && c != '\0') {
            filename.append(c);
        }

        // 构建完整文件路径
        QString fullPath;
        if (path.isEmpty()) {
            fullPath = QString("%1.%2").arg(QString(filename)).arg(QString(extension));
        } else {
            fullPath = QString("%1/%2.%3").arg(QString(path)).arg(QString(filename)).arg(QString(extension));
        }

        quint32 crc;

        // 读取文件条目
        VPKDirectoryEntry entry;
        in >> entry.crc >> entry.preloadBytes
            >> entry.archiveIndex >> entry.entryOffset
            >> entry.entryLength;

        // 跳过预加载数据（如果有）
        if (entry.preloadBytes > 0) {
            file.seek(file.pos() + entry.preloadBytes);
        }

        // 添加到结果列表
        ModFileInfo info;
        info.modName = QFileInfo(vpkPath).fileName();
        info.filePath = fullPath;
        info.crc32 = entry.crc;
        files.append(info);
    }

    file.close();
    return files;
}

/**
* @author   XiaoAn
* @brief    判断两个vpk文件是否冲突
* @date     2026-03-11
**/
bool ModConflictDetector::checkConflict(const QString &file1, const QString &file2)
{
    QList<ModFileInfo> modFileList1 = parseVPK(file1);
    QList<ModFileInfo> modFileList2 = parseVPK(file2);
    foreach (const ModFileInfo &modfile1, modFileList1) {
        foreach (const ModFileInfo &modfile2, modFileList2) {
            if(modfile1.filePath == modfile2.filePath &&
                modfile1.crc32 != modfile2.crc32){
                return true;
            }
        }
    }
    return false;
}
