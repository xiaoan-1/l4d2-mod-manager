#include "categorydialog.h"
#include "ui_categorydialog.h"

CategoryDialog::CategoryDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CategoryDialog)
{
    ui->setupUi(this);
}

CategoryDialog::~CategoryDialog()
{
    delete ui;
}

QString CategoryDialog::getCategory()
{
    return ui->lineEdit_category->text();
}
