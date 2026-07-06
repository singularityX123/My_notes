#ifndef EEGPROCESSINGWORKER_H
#define EEGPROCESSINGWORKER_H

#include <QObject>
#include <QVector>
#include <QImage>
#include <QtGlobal>
#include <limits>
#include "signalprocessor.h"
#include "ssvepfbcca.h"

struct ProcessingResult {
    quint64 epoch = 0;
    bool hasVisualData = false;
    QVector<double> channelPowers;
    QVector<double> psd;
    QImage  topomapImage;      // 工作线程预计算的热力图

    // FBCCA 算法结果（替代旧 SNR 峰值检测）
    FbccaResult fbcca;
};
Q_DECLARE_METATYPE(ProcessingResult)

class EEGProcessingWorker : public QObject
{
    Q_OBJECT

public:
    explicit EEGProcessingWorker(QObject *parent = nullptr);

    // 设置通道位置和标签（不随帧变化，只需设置一次）
    void setChannelInfo(const QVector<double> &positions,
                        const QVector<QString> &labels);

public slots:
    void processBlock(const QVector<QVector<double>> &block,
                      int sampleRate,
                      const QVector<int> &occipitalChannels,
                      double leftStimulusFreq,
                      double rightStimulusFreq,
                      quint64 epoch);

signals:
    void blockProcessed(const ProcessingResult &result);

private:
    SignalProcessor signalProcessor;
    SSVEPFbcca     m_fbcca;             // FBCCA 算法引擎
    QVector<QVector<double>> miChannelHistory;
    QVector<double> m_occipitalHistory;  // 枕叶平均信号历史（用于 PSD）
    int miHistorySampleRate = 0;
    quint64 lastEpoch = std::numeric_limits<quint64>::max();

    // 热力图参数（工作线程中缓存）
    QVector<double>  m_channelPositions;
    QVector<QString> m_channelLabels;
    int              m_topoGridSize = 128;
    double           m_topoValueRange = 0.0;   // 0=自动min-max归一化
};

#endif
