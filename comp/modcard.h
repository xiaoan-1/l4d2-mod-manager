#ifndef MODCARD_H
#define MODCARD_H

#include <QWidget>
#include <QCheckBox>
#include <QPushButton>

#include "../header.h"

namespace Ui {
class ModCard;
}
class ModCard : public QWidget
{
    Q_OBJECT
public:
    explicit ModCard(QWidget *parent = nullptr);
    ~ModCard();

    ModCard(const ModInfo &modInfo, QWidget *parent = nullptr);

public:
    // 获取Mod信息
    ModInfo modInfo() const;

    // 设置当前Mod分类
    void setCurrentCategory(const CategoryInfo &category);

    // 加载图片
    void loadImage(const QImage &image);

    // 刷新
    void updateModInfo();

    // 设置可选性
    void setCheckable(bool checkable);

    // 获取可选性
    bool checkable() const;

    // 获取选中状态
    bool isChecked() const;
private:

    // 备注
    void remark();

    // 转移
    void transfer();

    // 分类
    void classify();

protected:
    void hideEvent(QHideEvent* event) override;

    void showEvent(QShowEvent* event) override;

signals:
    // 隐藏信号
    void visiableChanged(bool visiable);

    // 销毁信号
    void destroyCard(const int &modId);
private:
    Ui::ModCard *ui;

    // 单选框
    QCheckBox *m_checkBox;

    // Mod文件信息
    ModInfo m_modInfo;

    // 当前分类
    CategoryInfo m_category;

    // 分类列表
    QList<CategoryInfo> m_categoryList;
};

#endif // MODCARD_H
