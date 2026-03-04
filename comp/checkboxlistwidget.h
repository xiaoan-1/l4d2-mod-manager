#ifndef CHECKBOXLISTWIDGET_H
#define CHECKBOXLISTWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QCheckBox>

namespace Ui {
class CheckBoxListWidget;
}

class CheckBoxListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CheckBoxListWidget(QWidget *parent = nullptr);
    ~CheckBoxListWidget();

    // 添加选项
    void addOption(const QString &option);
    void addOptions(const QStringList &options);

    // 移除选项
    void removeOption(const QString &option);

    // 设置选项的选中状态
    void setOptionChecked(const QString &option, const bool &checked);

    // 初始所有选项状态
    void resetOptionsChecked(const bool &checked);

signals:
    // 选项选中状态变化
    void checkStateChanged(const QMap<QString, bool> &optionCheckedState);

    void optionCheckStateChanged(const QString &option, const bool &checked);

private:
    Ui::CheckBoxListWidget *ui;

    // 控件列表
    QList<QWidget* > m_widgetList;

    // 选项选中状态
    QMap<QString, QCheckBox*> m_options;
};

#endif // CHECKBOXLISTWIDGET_H
