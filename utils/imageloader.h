#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include <QObject>
#include <QSize>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QSet>
#include <QMap>
#include <QElapsedTimer>
#include <QImage>
#include <QThread>
#include <QCache>
#include <QMetaType>

class ImageLoader : public QObject
{
    Q_OBJECT
public:
    // 图片加载优先级
    enum LoadPriority {
        PriorityLowest = 0,          // 最低优先级
        PriorityBackground = 10,    // 后台加载
        PriorityPreload = 20,       // 预加载
        PriorityNearVisible = 50,   // 即将可见区域
        PriorityVisible = 100      // 可见区域（最高）
    };
    // 图片加载任务信息
    struct Task {
        int id;              // 任务标识
        int priority;        // 优先级(数值越大优先级越高)
        int retryCount;      // 重试次数
        bool forceReload;    // 是否强制重新加载（忽略缓存）
        QString imagePath;   // 图片路径
        QSize targetSize;    // 目标大小
        qint64 timestamp;    // 任务创建时间戳

        Task() : priority(PriorityBackground), retryCount(0), timestamp(0){}
        // 比较操作符，用于排序
        bool operator<(const Task& other) const {
            if (priority != other.priority) {
                return priority > other.priority;
            }else{
                return timestamp < other.timestamp;
            }
        }
    };

    // 任务加载状态统计信息
    struct LoadStats {
        int totalTasks;           // 总任务数
        int completedTasks;       // 完成的任务数
        int failedTasks;          // 失败的任务数
        int cancelledTasks;       // 取消的任务数
        qint64 totalLoadTime;     // 总加载时间(ms)
        qint64 totalImageSize;    // 总图片大小(bytes)

        LoadStats() : totalTasks(0), completedTasks(0), failedTasks(0),
            cancelledTasks(0), totalLoadTime(0), totalImageSize(0) {}

        // 平均加载时间
        double averageLoadTime() const {
            return completedTasks > 0 ? static_cast<double>(totalLoadTime) / completedTasks : 0;
        }

        // 成功比例
        double successRate() const {
            return totalTasks > 0 ? static_cast<double>(completedTasks) / totalTasks * 100 : 0;
        }
    };

public:
    explicit ImageLoader(QObject *parent = nullptr);
    ~ImageLoader();

public:
    // 添加任务
    void addTask(const Task& task);
    // 添加多个任务
    void addTasks(const QList<Task> &tasks);

    // 取消任务
    void cancelTask(const int& taskId);
    // 取消多个任务
    void cancelTasks(const QList<int>& tasksId);

    // 清除任务
    void cancelAllTasks();

public:
    // 状态查询
    int pendingTaskCount() const;
    bool isPaused() const { return m_paused; }
    LoadStats getStats() const;
    void resetStats();

public slots:
    // 配置管理
    void setMaxCacheSize(int bytes);           // 设置缓存大小
    void setMaxConcurrentTasks(int count);     // 设置最大并发任务数
    void setLoadTimeout(int milliseconds);     // 设置加载超时时间
    void setRetryCount(int count);             // 设置失败重试次数

public:
    // 线程管理
    void start();           // 启动
    void pause();           // 暂停处理
    void resume();          // 恢复处理
    void stop();            // 停止（退出线程时调用）

signals:
    // 图片加载完毕
    void imageLoaded(int taskId, const QImage& image, bool fromCache);
    // 图片加载失败
    void imageLoadFailed(int taskId, const QString& error);
    // 任务进度
    void taskProgress(int pending, int active, int completed);
    // 任务数量变化
    void taskCountChanged(int pendingCount, int runningCount);
    // 任务状态更新
    void statsUpdated(const ImageLoader::LoadStats& stats);
    // 缓存占用变化
    void memoryUsageChanged(int cacheSize, int maxCacheSize);

private slots:
    void processTasks();


private:
    // 内部任务结构（包含运行时状态）
    struct RunningTask {
        Task task;
        QElapsedTimer timer;
        bool isRunning;

        RunningTask() : isRunning(false) {}
    };

    // 图片缓存项
    struct CachedImage {
        QImage image;
        qint64 loadTime;      // 加载耗时
        int accessCount;      // 访问次数
        qint64 lastAccess;    // 最后访问时间

        CachedImage() : loadTime(0), accessCount(0), lastAccess(0) {}
    };

private:
    // 加载图片文件
    bool loadImageFromFile(const Task& task, QImage& outImage, QString& errorMsg);
    // 验证图片
    bool validateImage(const QImage& image) const;
    // 优化图片大小
    QImage optimizeImage(const QImage& image, const QSize& targetSize) const;
    // 更新任务状态
    void updateStats(bool success, qint64 loadTime, int imageSize);
    // 清除缓存
    void cleanupCache();

private:
    // 统计信息
    LoadStats m_stats;

    // 线程管理
    mutable QMutex m_mutex;
    QWaitCondition m_condition;
    QVector<QThread*> m_workers;

    // 任务队列
    QList<Task> m_pendingTasks;                 // 按优先级排序的等待队列
    QMap<int, RunningTask> m_runningTasks;      // 正在执行的任务
    QSet<int> m_canceledTasks;                  // 已取消的任务

    // 缓存管理
    QMap<int, CachedImage> m_imageCache;        // 简单缓存，不使用QCache以便更精细控制
    int m_maxCacheSize;                         // 最大缓存大小(bytes)
    int m_currentCacheSize;                     // 当前缓存大小

    // 配置参数
    bool m_paused;                              // 是否暂停
    bool m_running;                             // 是否运行
    int m_maxConcurrentTasks;                   // 最大并发任务数
    int m_loadTimeoutMs;                        // 加载超时时间(ms)
    int m_maxRetryCount;                        // 最大重试次数

    static const int DEFAULT_RETRY = 2;                 // 默认重试2次
    static const int DEFAULT_CONCURRENT = 3;            // 默认3个并发任务
    static const int DEFAULT_TIMEOUT = 3000;            // 默认3秒超时
    static const int CACHE_CLEANUP_INTERVAL = 60000;    // 缓存清理间隔(1分钟)
};

#endif // IMAGELOADER_H
