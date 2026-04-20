#ifndef MODCARD_H
#define MODCARD_H

#include <QWidget>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>

#include "../header.h"

namespace Ui {
class ModCard;
}
class ModCard : public QWidget
{
    Q_OBJECT
public:
    enum class SizeMode{
        Normal,
        Small,
        Large,
        Conflict
    };
public:
    explicit ModCard(QWidget *parent = nullptr);
    ~ModCard();

    ModCard(const ModInfo &modInfo, QWidget *parent = nullptr, SizeMode sizeStyle = SizeMode::Normal);

public:
    // 设置大小模式
    void setSizeMode(SizeMode sizeMode);

    // 设置当前Mod分类
    void setCurrentCategory(const CategoryInfo &category);

    // 获取图片大小
    QSize getImageSize();

    // 是否存在分类
    bool hasCategory(const QString &catName);

    // 刷新模组信息
    void updateModInfo();

public slots:

    // 加载图片
    void loadImage(const QImage &image);

    // 设置图片错误信息
    void setImageErrorText(const QString &errorStr);

public:
    // 获取Mod信息
    ModInfo modInfo() const {return m_modInfo;};

    // 已分类列表
    QList<CategoryInfo> classfiedCategory() { return m_classifiedList; };

private:
    // 备注
    void remark();

    // 转移
    void transfer();

    // 分类
    void classify();

    // 移除分类和删除文件操作
    void remove();

protected:
    void enterEvent(QEnterEvent *event) override;

    void leaveEvent(QEvent *event) override;

    bool eventFilter(QObject *obj, QEvent *event) override;

signals:
    // 分类信号
    void classified();

    // 图片标齐全大小变化
    void imgResize();

    // 点击事件
    void clicked();

    // 启用与禁用信号
    void toggleDisabled(bool disabled);

    // 移除分类信号
    void removeCategory();

    // 删除文件信号
    void removeModFile();

private:
    Ui::ModCard *ui;

    // 移除/删除按钮
    QPushButton *m_removeButton = nullptr;

    // 备注编辑框
    QLineEdit *m_remarkEdit = nullptr;

    // Mod文件信息
    ModInfo m_modInfo;

    // 当前分类
    CategoryInfo m_category;

    // 该模组的所有分类
    QList<CategoryInfo> m_classifiedList;
};

#endif // MODCARD_H
