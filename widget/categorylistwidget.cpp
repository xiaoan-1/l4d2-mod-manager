#include "categorylistwidget.h"
#include "ui_categorylistwidget.h"

#include <QMimeData>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

CategoryListWidget::CategoryListWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CategoryListWidget)
{
    ui->setupUi(this);
    setAcceptDrops(true);
    m_layout = qobject_cast<QVBoxLayout*>(ui->scrollWidget->layout());
}

CategoryListWidget::~CategoryListWidget()
{
    delete ui;
}

/**
* @author   XiaoAn
* @brief    添加分类按钮，返回分类按钮对象
* @date     2026-04-02
**/
CategoryButton* CategoryListWidget::appendCategory(const CategoryInfo &category)
{
    CategoryButton *btn = new CategoryButton(category, this);
    m_layout->insertWidget(m_layout->count() - 1, btn);
    m_categoryButtons.append(btn);

    return btn;
}

/**
* @author   XiaoAn
* @brief    拖拽进入
* @date     2026-04-08
**/
void CategoryListWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (!event->mimeData()->hasFormat("custom/type")) {
        return;
    }
    foreach (auto btn, m_categoryButtons) {
        m_buttonPosList.append(btn->geometry().center());
    }

    // 读取指针字符串
    QByteArray ptrData = event->mimeData()->data("custom/type");
    quintptr ptr = ptrData.toULongLong();
    CategoryButton *catBtn = reinterpret_cast<CategoryButton*>(ptr);

    m_prevHoverIndex = m_categoryButtons.indexOf(catBtn);

    event->setDropAction(Qt::MoveAction);
    event->accept();

}

/**
* @author   XiaoAn
* @brief    拖拽移动
* @date     2026-04-08
**/
void CategoryListWidget::dragMoveEvent(QDragMoveEvent *event)
{
    if (!event->mimeData()->hasFormat("custom/type")) {
        return;
    }

    // 获取鼠标位置对应的插入索引
    int currHoverIndex = getInsertIndex(event->position().toPoint());

    // 如果悬停索引发生变化，执行实时交换动画
    if (m_prevHoverIndex != currHoverIndex) {
        // 实时交换位置
        addSortAnimationPair(m_prevHoverIndex, currHoverIndex);

        // 更新上一个悬停索引
        m_prevHoverIndex = currHoverIndex;
    }

    event->setDropAction(Qt::MoveAction);
    event->accept();
}

/**
* @author   XiaoAn
* @brief    拖拽松开
* @date     2026-04-08
**/
void CategoryListWidget::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasFormat("custom/type")) {
        return;
    }

    // 重新设置序号
    for (int i = 0; i < m_categoryButtons.size(); ++i) {
        m_categoryButtons[i]->setCategorySort(i);
    }
}

/**
* @author   XiaoAn
* @brief    获取插入的位置索引
* @date     2026-04-08
**/
int CategoryListWidget::getInsertIndex(const QPoint &pos)
{
    for (int i = 0; i < m_buttonPosList.size(); i++) {
        if (pos.y() < m_buttonPosList[i].y()) {
            return i;
        }
    }
    return m_categoryButtons.size() - 1;
}

/**
* @author   XiaoAn
* @brief    添加动画到队列中
* @date     2026-04-08
**/
void CategoryListWidget::addSortAnimationPair(int sourceIndex, int targetIndex)
{
    // 合并相邻的交换，优化队列
    if (!m_sortQueue.isEmpty()) {
        QPair<int, int> &last = m_sortQueue.last();
        // 如果连续的交换可以合并
        if (last.second == sourceIndex) {
            last.second = targetIndex;
            return;
        }
    }
    m_sortQueue.enqueue(qMakePair(sourceIndex, targetIndex));
    if (!m_isAnimating) {
        processSortAnimation();
    }
}

/**
* @author   XiaoAn
* @brief    执行排序动画
* @date     2026-04-08
**/
void CategoryListWidget::processSortAnimation()
{
    if (m_sortQueue.isEmpty()) {
        m_isAnimating = false;
        return;
    }

    m_isAnimating = true;
    QPair<int, int> swap = m_sortQueue.dequeue();
    int sourceIndex = swap.first;
    int targetIndex = swap.second;
    if (sourceIndex == targetIndex){
        processSortAnimation();
        return;
    }

    QPoint sourcePos = m_categoryButtons[sourceIndex]->pos();
    QPoint targetPos = m_categoryButtons[targetIndex]->pos();

    // 创建动画组
    QParallelAnimationGroup *group = new QParallelAnimationGroup(this);

    if(targetIndex > sourceIndex){
        // 上移一个位置
        // 0 - - 源位置 - [前移]- 目标位置 - - length
        for (int i = sourceIndex + 1; i <= targetIndex; ++i) {
            QPropertyAnimation *anim = new QPropertyAnimation(m_categoryButtons[i], "pos");
            anim->setDuration(100);
            anim->setStartValue(m_categoryButtons[i]->pos());
            anim->setEndValue(m_categoryButtons[i - 1]->pos());
            anim->setEasingCurve(QEasingCurve::Linear);
            group->addAnimation(anim);
        }
    }else{
        // 下移一个位置
        // 0 - - 目标位置 - [后移] - 源位置 - - length-1
        for (int i = sourceIndex - 1; i >= targetIndex; --i) {
            QPropertyAnimation *anim = new QPropertyAnimation(m_categoryButtons[i], "pos");
            anim->setDuration(100);
            anim->setStartValue(m_categoryButtons[i]->pos());
            anim->setEndValue(m_categoryButtons[i + 1]->pos());
            anim->setEasingCurve(QEasingCurve::Linear);
            group->addAnimation(anim);
        }
    }

    // 起始位置移动目标位置
    QPropertyAnimation *anim = new QPropertyAnimation(m_categoryButtons[sourceIndex], "pos");
    anim->setDuration(100);
    anim->setStartValue(sourcePos);
    anim->setEndValue(targetPos);
    anim->setEasingCurve(QEasingCurve::Linear);
    group->addAnimation(anim);

    // 动画完成后处理下一个
    connect(group, &QParallelAnimationGroup::finished, this, [=]() {
        // 更新数据结构
        m_categoryButtons.move(sourceIndex, targetIndex);

        group->deleteLater();
        // 处理队列中的下一个动画
        processSortAnimation();
    });
    group->start();
}
