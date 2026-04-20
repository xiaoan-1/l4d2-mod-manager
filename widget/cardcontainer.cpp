#include "cardcontainer.h"
#include "ui_cardcontainer.h"

#include <QSettings>
#include <QPixmapCache>
#include <windows.h>

#include "gamemanager.h"
#include "utils/imageloader.h"

CardContainer::CardContainer(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CardContainer)
{
    ui->setupUi(this);
}

CardContainer::~CardContainer()
{
    delete ui;
}

/**
* @author   XiaoAn
* @brief    刷新Mod卡片信息
* @date     2026-03-02
**/
void CardContainer::updateModCard()
{
    // 更新所有卡片信息
    for (auto it = m_modCardMap.begin(); it != m_modCardMap.end(); ++it) {
        ModCard *modCard = it.value();
        modCard->updateModInfo();
    }
}

/**
* @author   XiaoAn
* @brief    添加多个Mod卡片
* @date     2026-02-28
**/
void CardContainer::appendModCard(const QList<ModInfo> &modInfoList, const CategoryInfo &category)
{
    m_category = category;
    m_modInfoList = modInfoList;
    foreach (const ModInfo &modInfo, modInfoList) {
        ModCard *modCard = new ModCard(modInfo, ui->centralwidget, m_sizeMode);
        modCard->setFixedSize(m_cardSize);
        modCard->setCurrentCategory(category);
        m_modCardMap.insert(modInfo.id, modCard);

        // 设置分类信号, 应用过滤更新布局即可
        connect(modCard, &ModCard::classified, this, &CardContainer::updateLayout);

        // 启用和禁用
        connect(modCard, &ModCard::toggleDisabled, this, [=](){
            if(category.id == -1){
                // 非分类情况下需要移除卡片
                modCard->deleteLater();
                m_modCardMap.remove(modInfo.id);
            }
            updateLayout();
        });

        // 移除当前分类, 必然删除该模组卡片
        connect(modCard, &ModCard::removeCategory, this, [=](){
            modCard->deleteLater();
            m_modCardMap.remove(modInfo.id);
            updateLayout();
        });

        // 删除模组文件, 必然删除该模组卡片
        connect(modCard, &ModCard::removeModFile, this, [=](){
            modCard->deleteLater();
            m_modCardMap.remove(modInfo.id);
            updateLayout();
        });

        // 当模组卡片大小变化时提交图像加载任务
        connect(modCard, &ModCard::imgResize, this, [=](){
            ImageLoader::Task task;
            task.isCover = false;
            task.id = modInfo.id;
            task.imagePath = QString("%1/%2/%3.jpg").arg(
                GameManager::getInstance()->gamePath(), modInfo.relative_path , modInfo.original_name);
            task.targetSize = modCard->getImageSize();
            task.modCardPtr = modCard;
            ImageLoader::getInstance()->addTask(task);
        });
    }
    // 更新布局
    updateLayout();

}

/**
* @author   XiaoAn
* @brief    移除Mod卡片
* @date     2026-03-01
**/
void CardContainer::removeModCard(const int &modId)
{
    m_modCardMap.value(modId)->deleteLater();
    m_modCardMap.remove(modId);

    updateLayout();
}

/**
* @author   XiaoAn
* @brief    清空Mod卡片
* @date     2026-02-28
**/
void CardContainer::clearModCard()
{
    // 删除卡片
    for (auto it = m_modCardMap.begin(); it != m_modCardMap.end(); ++it) {
        it.value()->deleteLater();
    }
    m_modCardMap.clear();
}

/**
* @author   XiaoAn
* @brief    设置模组卡片的大小模式
* @date     2026-04-18
**/
void CardContainer::setCardSizeMode(ModCard::SizeMode sizeMode)
{
    m_sizeMode = sizeMode;
    if(sizeMode == ModCard::SizeMode::Normal){
        m_cardSize = {320, 300};
    }else if(sizeMode == ModCard::SizeMode::Conflict){
        m_cardSize = {220, 200};
    }
    for(auto it = m_modCardMap.begin(); it != m_modCardMap.end(); ++it)
    {
        it.value()->setFixedSize(m_cardSize);
    }
    updateLayout();
}

/**
* @author   XiaoAn
* @brief    是否显示禁用Mod
* @date     2026-04-12
**/
void CardContainer::setDisabledVisiable(bool visiable)
{
    m_disabledVisiable = visiable;
    updateLayout();
}

/**
* @author   XiaoAn
* @brief    搜索卡片
* @date     2026-03-01
**/
void CardContainer::slot_searchCard(const QString &name)
{
    m_searchFilter = name;
    updateLayout();
}

/**
* @author   XiaoAn
* @brief    设置分类是否显示
* @date     2026-03-31
**/
void CardContainer::slot_setCategoryFilter(const QString &catName, bool isShow)
{
    if(m_categoryFilter.contains(catName)){
        m_categoryFilter[catName] = isShow;
    }else {
        m_categoryFilter.insert(catName, isShow);
    }

    updateLayout();
}


/**
* @author   XiaoAn
* @brief    过滤卡片，同时匹配搜索和分类筛选
* @date     2026-03-15
**/
void CardContainer::filterCard()
{
    // 先获取筛选后的卡片列表
    for (auto it = m_modCardMap.begin(); it != m_modCardMap.end(); ++it) {
        ModCard *modCard = it.value();

        // 是否不显示禁用mod
        if(!m_disabledVisiable && modCard->modInfo().relative_path == GameManager::ModTrashDir){
            modCard->setVisible(false);
            continue;
        }

        // 该Mod卡片的分类是否被过滤
        bool isFiltered = false;
        for (auto cat = m_categoryFilter.begin(); cat != m_categoryFilter.end(); ++cat) {
            // 《未分类选项》被取消选择，没有分类的卡片将被过滤
            if(!cat.value() && cat.key() == "未分类" && modCard->classfiedCategory().isEmpty()){
                isFiltered = true;
                break;
            }

            // 该分类被取消选择，过滤存在该分类的卡片
            if(!cat.value() && modCard->hasCategory(cat.key())){
                isFiltered = true;
                break;
            }
        }

        // 被分类过滤则隐藏
        if(isFiltered){
            modCard->setVisible(false);
            continue;
        }

        if(m_searchFilter.isEmpty()){
            // 空搜索，显示全部
            modCard->setVisible(true);
            continue;
        }

        // 搜索过滤
        if(modCard->modInfo().custom_name.contains(m_searchFilter)
            || modCard->modInfo().original_name.contains(m_searchFilter)){
            modCard->setVisible(true);
        }else{
            modCard->setVisible(false);
        }
    }
}

/**
* @author   XiaoAn
* @brief    更新布局
* @date     2026-03-01
**/
void CardContainer::updateLayout()
{
    // 应用过滤条件
    filterCard();

    // 计算每行可以容纳的卡片数量(列数)
    int availableWidth = width() - m_leftMargin - m_rightMargin;
    int cardWithSpacing = m_cardSize.width() + m_horizontalSpacing;
    int colCount = (availableWidth + m_horizontalSpacing) / cardWithSpacing;
    if(colCount == 0){
        colCount = 1;
    }
    // 计算每个列额外的动态间距
    int extraSpacer = ((availableWidth + m_horizontalSpacing) % cardWithSpacing) / colCount;

    int col = 0;
    int x = m_leftMargin;
    int y = m_topMargin;
    int maxY = y;
    for (auto it = m_modCardMap.begin(); it != m_modCardMap.end(); ++it) {
        ModCard *modCard = it.value();

        if (!modCard->isVisibleTo(this)) continue;

        // 移动卡片到新位置
        modCard->move(x, y);
        maxY = qMax(maxY, y + m_cardSize.height());  // 更新最大 y 坐标
        // 下一列
        col++;
        if (col < colCount) {
            x += m_cardSize.width() + m_horizontalSpacing + extraSpacer;
        } else {
            // 换行
            col = 0;
            x = m_leftMargin;
            y += m_cardSize.height() + m_verticalSpacing;
        }
    }
    setFixedHeight(maxY + m_bottomMargin);
    emit updatedLayout();
}

/**
* @author   XiaoAn
* @brief    大小变化事件
* @date     2026-03-01
**/
void CardContainer::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // 重新布局
    if (event->size() != event->oldSize()) {
        updateLayout();
    }
}

/**
* @author   XiaoAn
* @brief    显示
* @date     2026-04-18
**/
void CardContainer::showEvent(QShowEvent *event)
{
    if(!m_modCardMap.isEmpty()){
        return;
    }
    // 重新加载卡片
    ImageLoader::getInstance()->start();

    clearModCard();
    appendModCard(m_modInfoList, m_category);
}

/**
* @author   XiaoAn
* @brief    隐藏
* @date     2026-04-18
**/
void CardContainer::hideEvent(QHideEvent *event)
{
    // 停止图片加载线程
    ImageLoader::getInstance()->stop();

    clearModCard();
    QPixmapCache::clear();

    QCoreApplication::processEvents();

    HeapCompact(GetProcessHeap(), 0);
}
