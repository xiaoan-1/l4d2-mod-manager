#ifndef MODCONFLICTDETECTOR_H
#define MODCONFLICTDETECTOR_H

#include <QObject>
#include <QMap>

// VPK目录条目结构 [citation:3]
struct VPKDirectoryEntry {
    quint32 crc;             // 文件的CRC32校验值 [citation:3]
    quint16 preloadBytes;     // 预加载数据大小 [citation:3]
    quint16 archiveIndex;     // 归档文件索引，0x7fff表示数据在目录文件中 [citation:3]
    quint32 entryOffset;      // 数据在归档文件中的偏移量 [citation:3]
    quint32 entryLength;      // 数据长度，如果为0则整个文件在预加载数据中 [citation:3]
    quint16 terminator;       // 终止符，固定为0xffff [citation:3]

    // 预加载数据紧随其后（长度为preloadBytes）
};

// 文件信息结构
struct ModFileInfo {
    QString modName;         // 所属mod名称
    QString filePath;        // 完整文件路径
    quint32 crc32;           // CRC32值
    bool hasPreload;         // 是否有预加载数据
};

class ModConflictDetector : public QObject
{
    Q_OBJECT
public:
    explicit ModConflictDetector(QObject *parent = nullptr);
    ~ModConflictDetector();

    // 解析vpk获取文件条目信息
    static QList<ModFileInfo> parseVPK(const QString& vpkPath);

    // 判断两个vpk文件是否冲突
    static bool checkConflict(const QString& file1, const QString& file2);
};

#endif // MODCONFLICTDETECTOR_H
