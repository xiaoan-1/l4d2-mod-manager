#ifndef CATEGORYBUTTON_H
#define CATEGORYBUTTON_H

#include <QWidget>
#include <QMenu>
#include <QPushButton>
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

private:
    void renameCategory();

    void deleteCategory();

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
};

#endif // CATEGORYBUTTON_H
