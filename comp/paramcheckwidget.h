#ifndef PARAMCHECKWIDGET_H
#define PARAMCHECKWIDGET_H

#include <QWidget>
#include <QCheckBox>
#include <QLineEdit>
#include <QMap>

#include "header.h"

namespace Ui {
class ParamCheckWidget;
}

class ParamCheckWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ParamCheckWidget(QWidget *parent = nullptr);
    ~ParamCheckWidget();

    void showCenter();

    void checkParam(const QList<QPair<QString, QString>> &paramList);

private:
    // 加载参数信息
    void loadParamInfo();

    // 添加参数信息
    void appendParamInfo(const QString &key, const QString &value, const QString &description);

    // 生成参数字符串
    void genGameParamStr();

private:
    Ui::ParamCheckWidget *ui;

    QMap<QString, QPair<QCheckBox*, QLineEdit*>> m_paramWidgetMap;


};

#endif // PARAMCHECKWIDGET_H
