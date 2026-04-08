#ifndef CATEGORYBUTTON_H
#define CATEGORYBUTTON_H

#include <QWidget>
#include <QMenu>
#include <QPushButton>
#include <QMouseEvent>
#include <QLineEdit>

#include "sqliteobj.h"

namespace Ui {
class CategoryButton;
}

class CategoryButton : public QWidget
{
    Q_OBJECT
public:
    explicit CategoryButton(QWidget *parent = nullptr);
    ~CategoryButton();

    CategoryButton(const CategoryInfo &category, QWidget *parent = nullptr);

public:
    QPushButton* coreButton() const;

    // 设置分类排序序号
    void setCategorySort(int sort);

private:
    void renameCategory();

    void deleteCategory();

protected:
    void mouseMoveEvent(QMouseEvent *event);

signals:
    void clicked(bool checked = false);

    void deleted();

    void renamed(const QString &oldName, const QString &newName);

private:
    Ui::CategoryButton *ui;

    // 选项菜单
    QMenu *m_menu = nullptr;

    // 输入框
    QLineEdit *m_lineEdit = nullptr;

    // 分类信息
    CategoryInfo m_category;

    // 拖拽起始位置
    QPoint m_dragStartPos;
};

#endif // CATEGORYBUTTON_H
