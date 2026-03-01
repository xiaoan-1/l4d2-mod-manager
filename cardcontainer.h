#ifndef CARDCONTAINER_H
#define CARDCONTAINER_H

#include <QWidget>

#include "comp/modcard.h"
#include "comp/imageloader.h"

namespace Ui {
class CardContainer;
}

class CardContainer : public QWidget
{
    Q_OBJECT

public:
    explicit CardContainer(QWidget *parent = nullptr);
    ~CardContainer();

    // 添加Mod卡片
    void appendModCard(const ModInfo &modInfo, const CategoryInfo &category = CategoryInfo());

    // 添加多个Mod卡片
    void appendModCard(const QList<ModInfo> &modInfoList, const CategoryInfo &category = CategoryInfo());


    // 清空Mod卡片
    void clearModCard();

private:

    void loadCardImage();

public slots:
    // 加载指定区域的卡片
    // void loadVisibleCards();

    // 暂停加载
    // void pauseLoading();

    // 恢复加载
    // void resumeLoading();

private slots:
    void onImageLoaded(const int& modId, const QImage& image, bool fromCache);

private:
    Ui::CardContainer *ui;

    QMap<int, ModCard*> m_modCardMap;
    // 图片加载器
    ImageLoader* m_imageLoader;
};

#endif // CARDCONTAINER_H
