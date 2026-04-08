#ifndef CATEGORYLISTWIDGET_H
#define CATEGORYLISTWIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QQueue>

#include "sqliteobj.h"
#include "comp/categorybutton.h"

namespace Ui {
class CategoryListWidget;
}

class CategoryListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CategoryListWidget(QWidget *parent = nullptr);
    ~CategoryListWidget();

    // 添加分类
    CategoryButton* appendCategory(const CategoryInfo &category);


protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);

private:
    // 获取坐标所在的分类按钮索引
    int getInsertIndex(const QPoint& pos);

    // 添加排序动画参数
    void addSortAnimationPair(int sourceIndex, int targetIndex);

    // 执行排序动画
    void processSortAnimation();

private:
    Ui::CategoryListWidget *ui;

    QVBoxLayout* m_layout;

    // 分类控件列表
    QVector<CategoryButton*> m_categoryButtons;

    // 分类控件的原始坐标
    QVector<QPoint> m_buttonPosList;

    // 拖拽相关
    int m_prevHoverIndex;
    bool m_isAnimating = false;
    QQueue<QPair<int, int>> m_sortQueue;
};

#endif // CATEGORYLISTWIDGET_H
