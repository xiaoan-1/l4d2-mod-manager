#ifndef MODCARD_H
#define MODCARD_H

#include <QWidget>

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
private:

    // 备注
    void remarkMod();

    // 移动Mod
    void moveMod();

    // 分类
    void classifyMod();

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

    // Mod文件信息
    ModInfo m_modInfo;

    // 当前分类
    CategoryInfo m_category;

    // 分类列表
    QList<CategoryInfo> m_categoryList;
};

#endif // MODCARD_H
