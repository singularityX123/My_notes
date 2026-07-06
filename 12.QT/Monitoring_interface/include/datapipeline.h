#ifndef DATAPIPELINE_H
#define DATAPIPELINE_H

#include <QObject>
#include <QVector>
#include <QThread>
#include <QElapsedTimer>
#include "eegdisplayfilter.h"
#include "eegprocessingworker.h"

// ============================================================================
// DataPipeline — EEG 数据处理管线协调器
// 单一责任：管理"原始数据 → 滤波缓冲 → Worker分派"的管线，通过信号输出结果
// ============================================================================

class DataPipeline : public QObject
{
    Q_OBJECT

public:
    explicit DataPipeline(QObject *parent = nullptr);
    ~DataPipeline();

    // 配置
    void setMIChannelIndex(int idx)       { m_miChannelIndex = idx; }
    void setOccipitalChannels(const QVector<int> &chs) { m_occipitalChannels = chs; }
    void setStimulusFreqs(double left, double right) { m_leftFreq = left; m_rightFreq = right; }

    // 从标签列表自动识别 MI(Cz) 和枕叶(O1/O2/Oz) 通道索引
    void initChannelMapping(const QStringList &labels);

    // 设置通道位置/标签（转发到工作线程，用于热力图计算）
    void setChannelInfo(const QVector<double> &positions,
                        const QVector<QString> &labels);

    // 设置运动想象标志（每次 trial annotation 更新时调用）
    void setInImagination(bool v) { m_inImagination = v; }

    // 重置管线状态（切换文件/跳转时调用）
    void reset();

    // 每帧主入口：处理一个数据块
    void processBlock(const QVector<QVector<double>> &block,
                      int sampleRate,
                      bool updateHeavyVisuals);

    // 波形缓冲区（只读访问，供 ChartManager 使用）
    const QVector<double> &waveformBuffer() const { return m_eegBuffer; }

    // 性能统计
    double averageProcessingTimeMs() const;

signals:
    // 请求 Worker 处理（内部信号，桥接到 Worker 线程）
    void requestProcessBlock(const QVector<QVector<double>> &block,
                             int sampleRate,
                             const QVector<int> &occipitalChannels,
                             double leftStimulusFreq,
                             double rightStimulusFreq,
                             quint64 epoch,
                             bool inImagination);

    // 转发通道信息到工作线程
    void requestSetChannelInfo(const QVector<double> &positions,
                               const QVector<QString> &labels);

    // 通知 MainWindow：波形缓冲已更新（每帧都触发）
    void waveformUpdated();

    // 通知 MainWindow：重量级处理结果已就绪（含 PSD + 通道功率 + SSVEP SNR）
    void processingResultReady(const ProcessingResult &result);

private slots:
    void onWorkerResult(const ProcessingResult &result);

private:
    void updateWaveformBuffer(const QVector<QVector<double>> &block, int sampleRate);

    // 处理管线
    QThread             *m_workerThread = nullptr;
    EEGProcessingWorker *m_worker       = nullptr;
    EEGDisplayFilter     m_filter;
    QVector<double>      m_eegBuffer;

    // 通道配置
    int          m_miChannelIndex  = -1;
    QVector<int> m_occipitalChannels;

    // SSVEP 参数
    double m_leftFreq  = 10.0;
    double m_rightFreq = 12.0;

    // Worker 协调
    bool    m_workerBusy = false;
    bool    m_inImagination = false;
    quint64 m_epoch      = 0;

    // 性能统计
    QElapsedTimer      m_frameTimer;
    QVector<double>    m_processingTimes;
    static constexpr int kMaxTimingSamples = 100;
};

#endif // DATAPIPELINE_H
