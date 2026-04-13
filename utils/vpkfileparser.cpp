#include "vpkfileparser.h"

#include <QFile>
#include <QFileInfo>
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
bool VpkFileParser::checkConflict(const VpkFileParser &vpkFile)
{
    foreach (const VPKFileEntry &entry1, m_entries) {
        foreach (const VPKFileEntry &entry2, vpkFile.entries()) {
            // 文件相同，但是校验值不同，说明文件冲突
            if(entry1.path == entry2.path && entry1.baseName == entry2.baseName &&
                entry1.extension == entry2.extension && entry1.crc32 != entry2.crc32){
                return true;
            }
        }
    }
    return false;
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
    qint64 treeStartPos = file.pos();
    qint64 treeEndPos = treeStartPos + m_vpkHeader.treeSize;

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
        fileEntry.path = path;
        fileEntry.baseName = filename;
        fileEntry.extension = extension;
        fileEntry.crc32 = dirEntry.crc;
        // 完整数据大小(预加载数据大小 + 文件数据区大小)
        fileEntry.entryLength = dirEntry.entryLength;
        // 预加载数据偏移量和大小
        fileEntry.preloadOffset = file.pos();
        fileEntry.preloadBytes = dirEntry.preloadBytes;
        // 文件数据区剩余数据偏移量（相对于文件数据区起始位置）
        fileEntry.remainingOffset = sizeof(VPKHeader) + m_vpkHeader.treeSize + dirEntry.entryOffset;

        m_entries.append(fileEntry);
    }
    file.close();
    return true;
}

