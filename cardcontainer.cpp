#include "cardcontainer.h"
#include "ui_cardcontainer.h"

#include "modmanager.h"

CardContainer::CardContainer(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CardContainer)
{
    ui->setupUi(this);


    // 创建图片加载器
    m_imageLoader = new ImageLoader();
    // 连接信号
    connect(m_imageLoader, &ImageLoader::imageLoaded, this, &CardContainer::onImageLoaded, Qt::QueuedConnection);

    m_imageLoader->start();
}

CardContainer::~CardContainer()
{
    m_imageLoader->stop();
    delete ui;
}

/**
* @author   XiaoAn
* @brief    添加多个Mod卡片
* @date     2026-02-28
**/
void CardContainer::appendModCard(const QList<ModInfo> &modInfoList, const CategoryInfo &category)
{
    m_imageLoader->cancelAllTasks();
    int row = m_modCardMap.size() / 4;
    int col = m_modCardMap.size() % 4;
    QGridLayout *gridLayout = qobject_cast<QGridLayout*>(ui->centralwidget->layout());


    QString gamePath = ModManager::getInstance()->gamePath();
    foreach (const ModInfo &modInfo, modInfoList) {
        ModCard *modCard = new ModCard(modInfo);
        modCard->setCurrentCategory(category);
        gridLayout->addWidget(modCard, row, col);

        m_modCardMap.insert(modInfo.id, modCard);

        // 提交Mod卡片图片加载任务
        ImageLoader::Task task;
        task.id = modInfo.id;
        task.imagePath = gamePath + modInfo.relative_path + "/" + modInfo.original_name + ".jpg";
        task.targetSize = QSize(300, 200);
        m_imageLoader->addTask(task);

        col++;
        if(col == 4){
            row++;
            col = 0;
        }
    }
}

/**
* @author   XiaoAn
* @brief    清空Mod卡片
* @date     2026-02-28
**/
void CardContainer::clearModCard()
{
    QGridLayout *gridLayout = qobject_cast<QGridLayout*>(ui->centralwidget->layout());

    while (gridLayout->count()) {
        gridLayout->takeAt(0)->widget()->deleteLater();
    }

    m_modCardMap.clear();
}

/**
* @author   XiaoAn
* @brief    加载图片
* @date     2026-02-28
**/
void CardContainer::onImageLoaded(const int &modId, const QImage &image, bool fromCache)
{
    if (!m_modCardMap.contains(modId)) return;
    // 更新卡片
    m_modCardMap.value(modId)->loadImage(image);
}
