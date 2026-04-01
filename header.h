#ifndef HEADER_H
#define HEADER_H

#include <QString>
#include <QDateTime>

// 分类信息结构体
struct CategoryInfo
{
    int id;                     // 唯一标识
    int sort;                   // 排序序号
    QString name;               // 分类名称
    QDateTime create_time;      // 创建时间

    CategoryInfo() : id(-1){}

    bool operator==(const CategoryInfo& other) const {
        return this->id == other.id;
    }
};

// Mod信息结构体
struct ModInfo {
    int id = -1;                // 唯一标识
    QString relative_path;      // Mod文件相对路径
    QString original_name;      // Mod文件名称
    QString custom_name;        // Mod自定义名称
    QDateTime create_time;      // 录入时间
    QString file_hash;          // 文件的哈希值

    bool operator==(const ModInfo& other) const {
        return this->id == other.id;
    }
};

#endif // HEADER_H
