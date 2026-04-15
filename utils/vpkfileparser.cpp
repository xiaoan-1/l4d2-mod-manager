#include "vpkfileparser.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QDebug>

VpkFileParser::VpkFileParser(const QString &filePath)
{
    m_filePath = filePath;
    parse();
}

/**
* @author   XiaoAn
* @brief    检测冲突
* @date     2026-04-14
**/
QList<QPair<int, int>> VpkFileParser::detectConflicts(const QList<VpkFileParser> &vpkFileList)
{
    // 1. 构建路径 -> (模组索引, CRC32) 列表的哈希表
    QHash<QString, QVector<QPair<int, quint32>>> pathToMods;

    for (int i = 0; i < vpkFileList.size(); ++i) {
        const auto& entries = vpkFileList[i].entries();
        for (const auto& entry : entries) {
            if(entry.path.isEmpty()) continue;
            QString fullPath = QString("%1/%2.%3").arg(entry.path, entry.baseName, entry.extension);
            pathToMods[fullPath].append(qMakePair(i, entry.crc32));
        }
    }

    // 2. 收集冲突模组对（去重）
    QSet<QPair<int, int>> conflictSet;
    for (auto it = pathToMods.begin(); it != pathToMods.end(); ++it) {
        const auto& modList = it.value();
        if (modList.size() < 2) continue;

        // 检查所有 CRC32 是否完全相同
        quint32 firstCrc = modList.first().second;
        bool allSame = true;
        for (const auto& p : modList) {
            if (p.second != firstCrc) {
                allSame = false;
                break;
            }
        }
        if (allSame) continue;  // 相同文件，不视为冲突

        // 记录所有模组对（无序，且去重）
        for (int i = 0; i < modList.size(); ++i) {
            for (int j = i + 1; j < modList.size(); ++j) {
                int a = modList[i].first;
                int b = modList[j].first;
                if (a > b) std::swap(a, b);
                conflictSet.insert(qMakePair(a, b));
            }
        }
    }

    // 转换为 QList 返回
    return conflictSet.values();
}

/**
* @author   XiaoAn
* @brief    获取文件条目数据
* @date     2026-04-14
**/
QByteArray VpkFileParser::getEntryFileData(const QString &filePath)
{
    QByteArray data;

    // 查找文件条目
    bool exist = false;
    VPKFileEntry fileEntry;
    foreach (const VPKFileEntry &entry, m_entries) {
        QString fullPath = QString("%1/%2.%3").arg(entry.path, entry.baseName, entry.extension);
        if(fullPath == filePath){
            exist = true;
            fileEntry = entry;
            break;
        }
    }

    if(!exist) return data;

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open file:" << m_filePath;
        return data;
    }
    data.reserve(fileEntry.entryLength);
    // 读取预加载数据
    if (fileEntry.preloadBytes > 0 && fileEntry.preloadOffset >= 0) {
        if (file.seek(fileEntry.preloadOffset)) {
            QByteArray preload = file.read(fileEntry.preloadBytes);
            data.append(preload);
        }
    }

    // 读取剩余数据
    quint32 remainLength = fileEntry.entryLength - fileEntry.preloadBytes;
    if ( remainLength > 0 && fileEntry.remainingOffset >= 0) {
        if (file.seek(fileEntry.remainingOffset)) {
            QByteArray remaining = file.read(remainLength);
            data.append(remaining);
        }
    }
    return data;
}

/**
* @author   XiaoAn
* @brief    获取模组信息
* @date     2026-04-14
**/
QMap<QString, QString> VpkFileParser::getAddonInfo()
{
    QMap<QString, QString> result;
    QString content = QString::fromUtf8(getEntryFileData("/addoninfo.txt"));

    if(content.isEmpty()) return result;

    // 1. 截取大括号内部的内容
    int braceStart = content.indexOf('{');
    if (braceStart == -1) return result;
    QString inner = content.mid(braceStart + 1);

    // 2. 按行分割
    QStringList lines = inner.split("\n", Qt::SkipEmptyParts);

    // 3. 逐行解析
    static const QRegularExpression keyValueSep(R"(\s+)");
    foreach (const QString &line , lines) {
        // 去除前后的/r和/n以及注释
        int commentPos = line.indexOf("//");
        QString trimmed = (commentPos == -1) ? line.trimmed() : line.left(commentPos).trimmed();
        // 跳过注释行和空行
        if (trimmed.isEmpty() || trimmed.startsWith("//"))
            continue;

        // 匹配第一个空白符、分隔的位置
        int firstSplit = trimmed.indexOf(keyValueSep);
        if (firstSplit == -1)
            continue;

        QString key = trimmed.left(firstSplit).trimmed().remove("\\").remove('\"');
        QString value = trimmed.mid(firstSplit).trimmed().remove('\\').remove('\"');
        result[key.toLower()] = value;
    }
    return result;
}

/**
* @author   XiaoAn
* @brief    解析vpk文件
* @date     2026-04-13
**/
bool VpkFileParser::parse()
{
    QFileInfo fileInfo(m_filePath);
    if(!fileInfo.exists() || fileInfo.suffix() != "vpk"){
        qWarning() << "not vpk file! " << m_filePath;
        return false;
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open file:" << m_filePath;
        return false;
    }
    QDataStream in(&file);
    // 使用小端字节序
    in.setByteOrder(QDataStream::LittleEndian);

    // ============================读取VPK文件头结构=============
    in >> m_vpkHeader.signature >> m_vpkHeader.version >> m_vpkHeader.treeSize;
    // 验证签名 (0x55AA1234)
    if (m_vpkHeader.signature != 0x55AA1234) {
        qWarning() << "Invalid VPK signature:" << m_filePath;
        file.close();
        return false;
    }
    // 如果是版本2，需要读取额外字段
    if (m_vpkHeader.version == 2) {
        in >> m_vpkHeader.fileDataSectionSize >> m_vpkHeader.archiveMD5SectionSize
            >> m_vpkHeader.otherMD5SectionSize >> m_vpkHeader.signatureSectionSize;
    }

    // ==========================解析目录树======================
    char c;
    QByteArray extension, path, filename;
    qint64 treeEndPos = file.pos() + m_vpkHeader.treeSize;

    // 按扩展名->路径->文件名 的三层*树形*结构遍历
    while (file.pos() < treeEndPos) {

        // 1. 读取扩展名
        extension.clear();
        while (file.getChar(&c) && c != '\0') {
            extension.append(c);
        }
        if (extension.isEmpty()) break; // 整个目录树结束

        while (true) {
            // 2. 读取扩展名下的多个路径
            path.clear();
            while (file.getChar(&c) && c != '\0') {
                path.append(c);
            }
            if (path.isEmpty()) break; // 当前扩展名结束

            while (true) {
                // 3. 读取文件名
                filename.clear();
                while (file.getChar(&c) && c != '\0') {
                    filename.append(c);
                }
                if (filename.isEmpty()) break; // 当前路径结束

                // 读取文件条目
                VPKDirectoryEntry dirEntry;
                in >> dirEntry.crc >> dirEntry.preloadBytes >> dirEntry.archiveIndex
                    >> dirEntry.entryOffset >> dirEntry.entryLength >> dirEntry.terminator;

                // 跳过预加载数据（如果有）
                if (dirEntry.preloadBytes > 0) {
                    file.seek(file.pos() + dirEntry.preloadBytes);
                }

                // 添加到结果列表
                VPKFileEntry fileEntry;
                fileEntry.path = path.trimmed();
                fileEntry.baseName = filename;
                fileEntry.extension = extension;
                fileEntry.crc32 = dirEntry.crc;
                // 完整数据大小(预加载数据大小 + 文件数据区大小)
                fileEntry.entryLength = dirEntry.entryLength;
                // 预加载数据偏移量和大小
                fileEntry.preloadOffset = file.pos();
                fileEntry.preloadBytes = dirEntry.preloadBytes;
                if (dirEntry.archiveIndex == 0x7FFF) {
                    // 文件数据区剩余数据偏移量（相对于文件数据区起始位置）
                    fileEntry.remainingOffset = 12 + m_vpkHeader.treeSize + dirEntry.entryOffset;
                }else{
                    fileEntry.remainingOffset = -1;
                }
                m_entries.append(fileEntry);
            }
        }
    }
    file.close();
    return true;
}

