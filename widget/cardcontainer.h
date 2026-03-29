#ifndef CARDCONTAINER_H
#define CARDCONTAINER_H

#include <QWidget>
#include <QResizeEvent>
#include "comp/modcard.h"
#include "utils/imageloader.h"

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

    // 筛选卡片
    void filterCard(const QString &catName, bool isShow);

    // 设置卡片大小
    void setCardFixedSize(const QSize& size);
    // 设置卡片之间的间距
    void setSpacing(int horizontal, int vertical);
    void setHorizontalSpacing(int spacing);
    void setVerticalSpacing(int spacing);

    // 设置边距
    void setContentsMargins(int left, int top, int right, int bottom);

public slots:    
    void slot_searchCard(const QString &name);

    void slot_disabledAll();

private:
    // 过滤卡片
    void filterCard();

    // 刷新布局
    void updateLayout();

private slots:
    void onImageLoaded(const int& modId, const QImage& image, bool fromCache);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::CardContainer *ui;

    // Mod卡片控件
    QMap<int, ModCard*> m_modCardMap;

    // 查询过滤
    QString m_searchFilter;

    // 分类筛选过滤
    QMap<QString, bool> m_categoryFilter;

    // 图片加载器
    ImageLoader* m_imageLoader;

    // 当前列数
    int m_colCount = 0;

    // 卡片固定大小
    QSize m_cardSize = {300, 300};

    // 间距
    int m_horizontalSpacing = 10;
    int m_verticalSpacing = 10;

    // 边距
    int m_leftMargin = 0;
    int m_topMargin = 0;
    int m_rightMargin = 0;
    int m_bottomMargin = 0;

    // 是否需要重新布局
    bool m_needsLayout = true;
};

#endif // CARDCONTAINER_H
