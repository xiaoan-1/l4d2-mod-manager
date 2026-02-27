#ifndef CATEGORYBUTTON_H
#define CATEGORYBUTTON_H

#include <QWidget>
#include <QPushButton>

namespace Ui {
class CategoryButton;
}

class CategoryButton : public QWidget
{
    Q_OBJECT
public:
    explicit CategoryButton(QWidget *parent = nullptr);
    ~CategoryButton();

    CategoryButton(const QString &name, QWidget *parent = nullptr);

public:
    QPushButton* coreButton() const;

signals:
    void deleteCategory();

signals:
    // 和QPushButton一致的信号
    void clicked(bool checked = false);
    void toggled(bool checked);
    // 属性变化信号
    void autoExclusiveChanged(bool exclusive);
    void checkableChanged(bool checkable);

private:
    Ui::CategoryButton *ui;
};

#endif // CATEGORYBUTTON_H
