#ifndef VPKFILEPARSER_H
#define VPKFILEPARSER_H

#include <QList>
#include <QString>

// VPK文件头
struct VPKHeader {
    quint32 signature;              // 固定值 0x55AA1234
    quint32 version;                // 版本号
    quint32 treeSize;               // 目录树的总字节数
    quint32 fileDataSectionSize;    // v2版本字段
    quint32 archiveMD5SectionSize;  // v2版本字段
    quint32 otherMD5SectionSize;
    quint32 signatureSectionSize;
};

// VPK目录条目结构
struct VPKDirectoryEntry {
    quint32 crc;                // 文件的CRC32校验值
    quint16 preloadBytes;       // 预加载数据大小
    quint16 archiveIndex;       // 数据文件索引（0 → _0.vpk，0x7FFF 表示无外部数据）
    quint32 entryOffset;        // 在数据文件中的偏移量（字节）
    quint32 entryLength;        // 文件的完整原始大小（字节）
    quint16 terminator;         // 终止符，固定为0xFFFF
};


struct VPKFileEntry {
    QString path;               // 文件路径
    QString baseName;           // 文件名称
    QString extension;          // 文件后缀
    uint32_t crc32;             // 文件 CRC32
    uint32_t entryLength;       // 文件原始总大小（字节）
    uint16_t preloadBytes;      // 预加载数据大小（字节）
    qint64 preloadOffset;       // 预加载数据在文件中的偏移（若无则为 -1）
    qint64 remainingOffset;     // 剩余数据在文件中的偏移（若无则为 -1）
};

class VpkFileParser
{
public:
    explicit VpkFileParser(const QString &filePath);

public:
    // 获取vpk文件内部文件条目信息
    QList<VPKFileEntry> entries() const { return m_entries; } ;

    // 检测冲突
    bool checkConflict(const VpkFileParser &vpkFile);

private:
    // 解析文件
    bool parse();

private:
    QString m_filePath;

    // 文件头部结构
    VPKHeader m_vpkHeader;

    // 文件目录条目
    QList<VPKFileEntry> m_entries;

};

#endif // VPKFILEPARSER_H
