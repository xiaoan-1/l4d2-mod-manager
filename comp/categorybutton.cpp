#include "categorybutton.h"
#include "ui_categorybutton.h"

CategoryButton::CategoryButton(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CategoryButton)
{
    ui->setupUi(this);
}

CategoryButton::~CategoryButton()
{
    delete ui;
}

CategoryButton::CategoryButton(const QString &name, QWidget *parent)
    : CategoryButton(parent)
{
    ui->pushButton_name->setText(name);

    connect(ui->pushButton_name, &QPushButton::clicked, this, &CategoryButton::clicked);

    connect(ui->pushButton_delete, &QPushButton::clicked, this, &CategoryButton::deleteCategory);

}

QPushButton *CategoryButton::coreButton() const
{
    return ui->pushButton_name;
}
