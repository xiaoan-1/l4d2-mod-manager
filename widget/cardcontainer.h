#ifndef CARDCONTAINER_H
#define CARDCONTAINER_H

#include <QWidget>
#include <QResizeEvent>
#include "comp/modcard.h"


namespace Ui {
class CardContainer;
}

class CardContainer : public QWidget
{
    Q_OBJECT

public:
    explicit CardContainer(QWidget *parent = nullptr);
    ~CardContainer();

    // 刷新Mod卡片信息
    void updateModCard();

    // 添加Mod卡片
    void appendModCard(const QList<ModInfo> &modInfoList, const CategoryInfo &category = CategoryInfo());

    // 移除Mod卡片
    void removeModCard(const int &modId);

    // 清空Mod卡片
    void clearModCard();

public:
    // 设置卡片大小
    void setCardSizeMode(ModCard::SizeMode sizeMode);

    // 是否显示禁用mod
    void setDisabledVisiable(bool visiable);

public:
    QList<ModCard*> modCardList() { return m_modCardMap.values(); };

public slots:    
    void slot_searchCard(const QString &name);

    void slot_setCategoryFilter(const QString &catName, bool isShow);

private:
    // 过滤卡片
    void filterCard();

    // 刷新布局
    void updateLayout();

protected:
    void resizeEvent(QResizeEvent *event) override;

    void showEvent(QShowEvent *event) override;

    void hideEvent(QHideEvent *event) override;

signals:
    void updatedLayout();

private:
    Ui::CardContainer *ui;

    // 模组分类及其列表
    CategoryInfo m_category;
    QList<ModInfo> m_modInfoList;

    // Mod卡片控件
    QMap<int, ModCard*> m_modCardMap;

    // 查询过滤
    QString m_searchFilter;

    // 是否显示禁用mod
    bool m_disabledVisiable = true;

    // 分类筛选过滤
    QMap<QString, bool> m_categoryFilter;

    // 大小
    ModCard::SizeMode m_sizeMode = ModCard::SizeMode::Normal;

    // 卡片固定大小
    QSize m_cardSize = {320, 300};

    // 间距
    int m_horizontalSpacing = 10;
    int m_verticalSpacing = 10;

    // 边距
    int m_leftMargin = 0;
    int m_topMargin = 0;
    int m_rightMargin = 0;
    int m_bottomMargin = 0;
};

#endif // CARDCONTAINER_H
