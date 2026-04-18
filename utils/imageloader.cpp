#include "imageloader.h"

#include <QDir>
#include <QBuffer>
#include <QDateTime>
#include <QFileInfo>
#include <QImageReader>
#include <QDebug>

#include "./vpkfileparser.h"

ImageLoader::ImageLoader(QObject *parent)
    : QObject{parent}
    , m_maxCacheSize(10 * 1024 * 1024)  // 默认50MB缓存
    , m_currentCacheSize(0)
    , m_maxConcurrentTasks(DEFAULT_CONCURRENT)
    , m_loadTimeoutMs(DEFAULT_TIMEOUT)
    , m_maxRetryCount(DEFAULT_RETRY)
    , m_paused(false)
    , m_running(true)
{
    // 最大线程数
    for (int i = 0; i < m_maxConcurrentTasks; ++i) {
        QThread* thread = new QThread(this);
        connect(thread, &QThread::started, thread, [this, thread]() {
            processTasks();
        }, Qt::DirectConnection);
        m_workers.append(thread);
    }
}

ImageLoader::~ImageLoader()
{
    stop();
}


ImageLoader *ImageLoader::getInstance(QObject *parent)
{
    static ImageLoader* instance = nullptr;
    static QMutex mutex;
    if (!instance) {
        QMutexLocker locker(&mutex);
        if (!instance) {
            instance = new ImageLoader(parent);
            instance->start();
        }
    }
    return instance;
}

/**
* @author   XiaoAn
* @brief    提交任务
* @date     2026-02-28
**/
void ImageLoader::addTask(const Task &task)
{
    QMutexLocker locker(&m_mutex);

    // 检查是否已存在相同的任务
    for (int i = 0; i < m_pendingTasks.size(); ++i) {
        if (m_pendingTasks[i].id == task.id && task.isCover) {
            m_pendingTasks.remove(i);
            break;
        }
    }

    // 检查是否已在运行中
    // if (m_runningTasks.contains(task.id)) {
    //     RunningTask& rt = m_runningTasks[task.id];
    //     if (task.priority > rt.task.priority) {
    //         // 优先级更高，可做中断处理
    //     }
    //     return;
    // }

    // 设置时间戳
    Task newTask = task;
    newTask.timestamp = QDateTime::currentMSecsSinceEpoch();

    // 按优先级插入（优先级高的在前）
    auto it = std::lower_bound(m_pendingTasks.begin(), m_pendingTasks.end(), newTask);
    m_pendingTasks.insert(it, newTask);

    emit taskCountChanged(m_pendingTasks.size(), m_runningTasks.size());

    // 唤醒处理线程
    m_condition.wakeOne();
}

/**
* @author   XiaoAn
* @brief    批量添加任务
* @date     2026-02-28
**/
void ImageLoader::addTasks(const QList<Task> &tasks)
{
    QMutexLocker locker(&m_mutex);

    foreach (const Task& task, tasks) {
        // 批量添加，简化处理
        Task newTask = task;
        newTask.timestamp = QDateTime::currentMSecsSinceEpoch();

        auto it = std::lower_bound(m_pendingTasks.begin(), m_pendingTasks.end(), newTask);
        m_pendingTasks.insert(it, newTask);
    }

    emit taskCountChanged(m_pendingTasks.size(), m_runningTasks.size());
    m_condition.wakeOne();
}

/**
* @author   XiaoAn
* @brief    取消任务
* @date     2026-02-28
**/
void ImageLoader::cancelTask(const int &taskId)
{
    QMutexLocker locker(&m_mutex);

    // 从等待队列中移除
    for (int i = 0; i < m_pendingTasks.size(); ++i) {
        if (m_pendingTasks[i].id == taskId) {
            m_pendingTasks.removeAt(i);
            m_stats.cancelledTasks++;
            break;
        }
    }

    // 标记取消正在运行的任务
    m_canceledTasks.insert(taskId);

    emit taskCountChanged(m_pendingTasks.size(), m_runningTasks.size());
}

/**
* @author   XiaoAn
* @brief    批量取消任务
* @date     2026-02-28
**/
void ImageLoader::cancelTasks(const QList<int> &tasksId)
{
    QMutexLocker locker(&m_mutex);

    foreach (const int& taskId, tasksId) {
        for (int i = 0; i < m_pendingTasks.size(); ++i) {
            if (m_pendingTasks[i].id == taskId) {
                m_pendingTasks.removeAt(i);
                m_stats.cancelledTasks++;
                break;
            }
        }
        m_canceledTasks.insert(taskId);
    }

    emit taskCountChanged(m_pendingTasks.size(), m_runningTasks.size());
}

/**
* @author   XiaoAn
* @brief    取消所有任务
* @date     2026-02-28
**/
void ImageLoader::cancelAllTasks()
{
    QMutexLocker locker(&m_mutex);

    m_stats.cancelledTasks += m_pendingTasks.size();
    m_pendingTasks.clear();

    foreach (const int& taskId, m_runningTasks.keys()) {
        m_canceledTasks.insert(taskId);
    }

    emit taskCountChanged(m_pendingTasks.size(), m_runningTasks.size());
}

/**
* @author   XiaoAn
* @brief    等待任务数量
* @date     2026-02-28
**/
int ImageLoader::pendingTaskCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_pendingTasks.size();
}

/**
* @author   XiaoAn
* @brief    任务加载统计信息
* @date     2026-02-28
**/
ImageLoader::LoadStats ImageLoader::getStats() const
{
    QMutexLocker locker(&m_mutex);
    return m_stats;
}

/**
* @author   XiaoAn
* @brief    重置状态
* @date     2026-02-28
**/
void ImageLoader::resetStats()
{
    QMutexLocker locker(&m_mutex);
    m_stats = LoadStats();
}

/**
* @author   XiaoAn
* @brief    设置缓存最大容量，单位byte
* @date     2026-02-28
**/
void ImageLoader::setMaxCacheSize(int bytes)
{
    QMutexLocker locker(&m_mutex);
    m_maxCacheSize = bytes;
    cleanupCache();
}

/**
* @author   XiaoAn
* @brief    设置最大任务数
* @date     2026-02-28
**/
void ImageLoader::setMaxConcurrentTasks(int count)
{
    QMutexLocker locker(&m_mutex);
    m_maxConcurrentTasks = qMax(1, count);
}

/**
* @author   XiaoAn
* @brief    设置图片加载超时时间
* @date     2026-02-28
**/
void ImageLoader::setLoadTimeout(int milliseconds)
{
    QMutexLocker locker(&m_mutex);
    m_loadTimeoutMs = milliseconds;
}

/**
* @author   XiaoAn
* @brief    设置图片加载失败最大的重置次数
* @date     2026-02-28
**/
void ImageLoader::setRetryCount(int count)
{
    QMutexLocker locker(&m_mutex);
    m_maxRetryCount = qMax(0, count);
}

/**
* @author   XiaoAn
* @brief    启动线程
* @date     2026-03-01
**/
void ImageLoader::start()
{
    foreach (const auto &thread, m_workers) {
        thread->start();
    }
}


/**
* @author   XiaoAn
* @brief    暂停线程
* @date     2026-02-28
**/
void ImageLoader::pause()
{
    QMutexLocker locker(&m_mutex);
    m_paused = true;
}

/**
* @author   XiaoAn
* @brief    恢复线程
* @date     2026-02-28
**/
void ImageLoader::resume()
{
    QMutexLocker locker(&m_mutex);
    m_paused = false;
    m_condition.wakeAll();
}

/**
* @author   XiaoAn
* @brief    停止线程
* @date     2026-02-28
**/
void ImageLoader::stop()
{
    QMutexLocker locker(&m_mutex);
    m_running = false;
    m_condition.wakeAll();
}

/**
* @author   XiaoAn
* @brief    执行任务
* @date     2026-02-28
**/
void ImageLoader::processTasks()
{
    while (m_running) {
        // 检查是否暂停
        if (m_paused) {
            QMutexLocker locker(&m_mutex);
            m_condition.wait(&m_mutex, 1000);
            continue;
        }

        // 1. 先检查并清理运行中超时的任务
        {
            QMutexLocker locker(&m_mutex);
            // 检查运行中的任务是否超时,移除超时任务
            for (auto it = m_runningTasks.begin(); it != m_runningTasks.end(); ) {
                if (it.value().timer.elapsed() > m_loadTimeoutMs) {
                    Task task = it.value().task;
                    if(task.taskTargetPtr){
                        emit imageLoadFailedByPtr(task.taskTargetPtr, "加载超时");
                    }else{
                        emit imageLoadFailedById(task.id, "加载超时");
                    }
                    it = m_runningTasks.erase(it);
                } else {
                    ++it;
                }
            }
        }

        // 从等待队列中取出任务到运行中任务列表
        Task task;
        {
            QMutexLocker locker(&m_mutex);

            // 等待任务完成
            if (m_runningTasks.size() >= m_maxConcurrentTasks) {
                m_condition.wait(&m_mutex, 1000);
                continue;
            }

            // 没有任务时等待
            if (m_pendingTasks.isEmpty()) {
                m_condition.wait(&m_mutex, 5000);
                continue;
            }

            // 取出下一个任务
            task = m_pendingTasks.takeFirst();

            // 检查是否已被取消
            if (m_canceledTasks.contains(task.id)) {
                // 从取消列表中移除并重新触发任务
                m_canceledTasks.remove(task.id);
                m_stats.cancelledTasks++;
                continue;
            }

            // 检查缓存
            if (!task.forceReload && m_imageCache.contains(task.id)) {
                CachedImage& cached = m_imageCache[task.id];
                cached.accessCount++;
                cached.lastAccess = QDateTime::currentMSecsSinceEpoch();
                // 缓存命中，任务完成
                if(task.taskTargetPtr){
                    emit imageLoadedByPtr(task.taskTargetPtr, cached.image, true);
                }else{
                    emit imageLoadedById(task.id, cached.image, true);
                }
                m_stats.completedTasks++;

                emit taskCountChanged(m_pendingTasks.size(), m_runningTasks.size());
                continue;
            }

            // 添加到运行中
            RunningTask rt;
            rt.task = task;
            rt.isRunning = true;
            rt.timer.start();
            m_runningTasks[task.id] = rt;
        }

        // 加载图片
        QImage image;
        QString errorMsg;
        bool success = loadImageFromFile(task, image, errorMsg);

        // 处理运行中任务
        {
            QMutexLocker locker(&m_mutex);
            if (!m_runningTasks.contains(task.id)) {
                // 任务已被取消
                continue;
            }
            // 获取运行中任务
            RunningTask rt = m_runningTasks.take(task.id);
            qint64 loadTime = rt.timer.elapsed();

            if (!m_canceledTasks.contains(task.id)) {
                if (success && validateImage(image)) {
                    // 存入缓存
                    CachedImage cached;
                    cached.image = image;
                    cached.loadTime = loadTime;
                    cached.accessCount = 1;
                    cached.lastAccess = QDateTime::currentMSecsSinceEpoch();

                    int imageSize = image.sizeInBytes();
                    m_currentCacheSize += imageSize;
                    m_imageCache.insert(task.id, cached);

                    // 如果超出限制清理缓存
                    cleanupCache();
                    if(task.taskTargetPtr){
                        emit imageLoadedByPtr(task.taskTargetPtr, cached.image, false);
                    }else{
                        emit imageLoadedById(task.id, image, false);
                    }

                    // 更新统计
                    updateStats(true, loadTime, imageSize);
                } else if (task.retryCount < m_maxRetryCount) {

                    // 重新添加任务队列头中
                    task.retryCount++;
                    m_pendingTasks.prepend(task);
                } else {
                    // 最终失败
                    if(task.taskTargetPtr){
                        emit imageLoadFailedByPtr(task.taskTargetPtr, errorMsg);
                    }else{
                        emit imageLoadFailedById(task.id, errorMsg);
                    }
                    updateStats(false, loadTime, 0);
                }
            } else {
                m_canceledTasks.remove(task.id);
                m_stats.cancelledTasks++;
            }

            emit taskCountChanged(m_pendingTasks.size(), m_runningTasks.size());
            emit memoryUsageChanged(m_currentCacheSize, m_maxCacheSize);
        }
        // 避免CPU占用过高
        QThread::msleep(10);
    }
}

/**
* @author   XiaoAn
* @brief    加载图片文件
* @date     2026-02-28
**/
bool ImageLoader::loadImageFromFile(const Task &task, QImage &outImage, QString &errorMsg)
{
    QFileInfo fileInfo(task.imagePath);

    // 检查图片文件是否存在
    if (!fileInfo.exists()) {
        // 转为解析vpk内部图片数据
        QString vpkFilePath = QString("%1/%2.vpk").arg(fileInfo.dir().absolutePath(), fileInfo.baseName());
        fileInfo.setFile(vpkFilePath);
        if(!fileInfo.exists()){
            errorMsg = "[ERROR] 文件不存在";
            return false;
        }
    }
    QImageReader reader;
    // 设置读取选项
    reader.setFormat("jpg");
    reader.setAutoTransform(true);
    reader.setDecideFormatFromContent(true);

    QBuffer buffer;
    QByteArray imageData;
    if(fileInfo.suffix() == "jpg"){
        // 直接读取jpg图像
        reader.setFileName(task.imagePath);
    }else if (fileInfo.suffix() == "vpk"){
        // 读取vpk内部图像数据
        VpkFileParser vpkFile(fileInfo.absoluteFilePath());
        imageData = vpkFile.getEntryFileData("/addonimage.jpg");
        if(imageData.isEmpty()){
            errorMsg = "[ERROR] 未读取到vpk内部图片";
            return false;
        }
        // 使用 QBuffer 将 QByteArray 包装为 QIODevice
        buffer.setBuffer(&imageData);
        buffer.open(QIODevice::ReadOnly);
        reader.setDevice(&buffer);
    }
    // 如果有目标大小，可以先读取缩略图提高效率
    if (!task.targetSize.isEmpty() && task.targetSize.width() > 0) {
        QSize imageSize = reader.size();
        if (imageSize.isValid()) {
            // 计算合适的缩放比例
            QSize scaledSize = reader.size().scaled(task.targetSize, Qt::KeepAspectRatio);
            reader.setScaledSize(scaledSize);
        }
    }
    // 读取图片
    if (!reader.read(&outImage)) {
        errorMsg = "[ERROR] 图像数据解析失败！";
        return false;
    }
    return true;
}

/**
* @author   XiaoAn
* @brief    校验图片文件
* @date     2026-02-28
**/
bool ImageLoader::validateImage(const QImage &image) const
{
    if (image.isNull()) return false;
    if (image.width() <= 0 || image.height() <= 0) return false;

    // 检查图像格式
    if (image.format() == QImage::Format_Invalid) return false;

    return true;
}

/**
* @author   XiaoAn
* @brief    更新状态
* @date     2026-02-28
**/
void ImageLoader::updateStats(bool success, qint64 loadTime, int imageSize)
{
    m_stats.totalTasks++;
    if (success) {
        m_stats.completedTasks++;
        m_stats.totalLoadTime += loadTime;
        m_stats.totalImageSize += imageSize;
    } else {
        m_stats.failedTasks++;
    }

    emit statsUpdated(m_stats);
}

/**
* @author   XiaoAn
* @brief    清除缓存
* @date     2026-02-28
**/
void ImageLoader::cleanupCache()
{
    if (m_currentCacheSize <= m_maxCacheSize) return;

    // 读取缓存列表
    QList<QPair<int, CachedImage>> items;
    for (auto it = m_imageCache.begin(); it != m_imageCache.end(); ++it) {
        items.append(qMakePair(it.key(), it.value()));
    }

    // 按最后访问时间排序（最早的在前）
    std::sort(items.begin(), items.end(),
              [](const QPair<int, CachedImage>& a,const QPair<int, CachedImage>& b) {
                  return a.second.lastAccess < b.second.lastAccess;
              });

    // 删除最旧的项目直到低于缓存限制
    int targetSize = m_maxCacheSize * 0.8;  // 保留80%的目标大小，避免频繁清理
    while (m_currentCacheSize > targetSize && !items.isEmpty()) {
        auto item = items.takeFirst();
        m_currentCacheSize -= item.second.image.sizeInBytes();
        m_imageCache.remove(item.first);
    }
}
