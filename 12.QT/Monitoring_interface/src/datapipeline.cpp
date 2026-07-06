#include "datapipeline.h"
#include <QMetaType>
#include <algorithm>

DataPipeline::DataPipeline(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<ProcessingResult>("ProcessingResult");
    qRegisterMetaType<QVector<QVector<double>>>("QVector<QVector<double>>");

    m_workerThread = new QThread(this);
    m_worker = new EEGProcessingWorker();
    m_worker->moveToThread(m_workerThread);

    connect(this, &DataPipeline::requestProcessBlock,
            m_worker, &EEGProcessingWorker::processBlock,
            Qt::QueuedConnection);
    connect(this, &DataPipeline::requestSetChannelInfo,
            m_worker, &EEGProcessingWorker::setChannelInfo,
            Qt::QueuedConnection);
    connect(m_worker, &EEGProcessingWorker::blockProcessed,
            this, &DataPipeline::onWorkerResult,
            Qt::QueuedConnection);
    connect(m_workerThread, &QThread::finished,
            m_worker, &QObject::deleteLater);

    m_workerThread->start();
}

DataPipeline::~DataPipeline()
{
    if (m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
}

void DataPipeline::setChannelInfo(const QVector<double> &positions,
                                   const QVector<QString> &labels)
{
    emit requestSetChannelInfo(positions, labels);
}

void DataPipeline::reset()
{
    ++m_epoch;
    m_workerBusy = false;
    m_filter.reset();
    m_eegBuffer.clear();
    m_processingTimes.clear();
}

void DataPipeline::processBlock(const QVector<QVector<double>> &block,
                                 int sampleRate,
                                 bool updateHeavyVisuals)
{
    if (block.isEmpty() || block[0].isEmpty())
        return;

    m_frameTimer.start();

    // 1) 轻量：更新波形缓冲区（每帧都执行）
    updateWaveformBuffer(block, sampleRate);
    emit waveformUpdated();

    if (!updateHeavyVisuals)
        return;

    // 2) 重量级：分派到 Worker 线程做 FFT/PSD/SNR
    if (m_workerBusy || sampleRate <= 0)
        return;

    m_workerBusy = true;
    emit requestProcessBlock(block, sampleRate, m_occipitalChannels,
                             m_leftFreq, m_rightFreq, m_epoch, m_inImagination);
}

void DataPipeline::onWorkerResult(const ProcessingResult &result)
{
    m_workerBusy = false;

    // 性能记录
    m_processingTimes.append(m_frameTimer.elapsed());
    if (m_processingTimes.size() > kMaxTimingSamples)
        m_processingTimes.removeFirst();

    if (result.epoch != m_epoch || !result.hasVisualData)
        return;

    // 通过信号分发结果，由 MainWindow 决定如何可视化
    emit processingResultReady(result);
}

void DataPipeline::initChannelMapping(const QStringList &labels)
{
    m_miChannelIndex = -1;
    m_occipitalChannels.clear();

    for (int i = 0; i < labels.size(); ++i) {
        const QString norm = labels.at(i).trimmed().toUpper();
        if (norm == "CZ")
            m_miChannelIndex = i;
        if (norm == "O1" || norm == "O2" || norm == "OZ")
            m_occipitalChannels.append(i);
    }

    if (m_miChannelIndex < 0 && !labels.isEmpty())
        m_miChannelIndex = 0;

    if (m_occipitalChannels.isEmpty() && !labels.isEmpty()) {
        m_occipitalChannels.append(qMax(0, labels.size() - 1));
        if (labels.size() > 1)
            m_occipitalChannels.append(qMax(0, labels.size() - 2));
    }
}

void DataPipeline::updateWaveformBuffer(const QVector<QVector<double>> &block,
                                         int sampleRate)
{
    if (m_miChannelIndex < 0 || m_miChannelIndex >= block.size())
        return;

    sampleRate = qMax(1, sampleRate);
    m_filter.configure(sampleRate);

    const QVector<double> &segment = block.at(m_miChannelIndex);
    for (int i = 0; i < segment.size(); ++i)
        m_eegBuffer.append(m_filter.filter(segment.at(i)));

    // 保留最近 4 秒
    const int maxSamples = sampleRate * 4;
    while (m_eegBuffer.size() > maxSamples)
        m_eegBuffer.removeFirst();
}

double DataPipeline::averageProcessingTimeMs() const
{
    if (m_processingTimes.isEmpty())
        return 0.0;
    double sum = 0.0;
    for (double t : m_processingTimes)
        sum += t;
    return sum / m_processingTimes.size();
}
