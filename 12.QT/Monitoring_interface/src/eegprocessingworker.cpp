#include "eegprocessingworker.h"
#include "mnetopomap.h"
#include <algorithm>

EEGProcessingWorker::EEGProcessingWorker(QObject *parent)
    : QObject(parent)
    , signalProcessor(nullptr)
    , m_fbcca(nullptr)
{
    // 默认 FBCCA 配置：10Hz/12Hz 刺激，3 次谐波
    m_fbcca.setStimulusFreqs(10.0, 12.0);
    m_fbcca.setNumHarmonics(3);
    m_fbcca.setScoreScale(20.0);
    m_fbcca.setDecisionThreshold(0.15);
    m_fbcca.setMinSamples(64);
}

void EEGProcessingWorker::setChannelInfo(const QVector<double> &positions,
                                          const QVector<QString> &labels)
{
    m_channelPositions = positions;
    m_channelLabels    = labels;
}

void EEGProcessingWorker::processBlock(const QVector<QVector<double>> &block,
                                       int sampleRate,
                                       const QVector<int> &occipitalChannels,
                                       double leftStimulusFreq,
                                       double rightStimulusFreq,
                                       quint64 epoch)
{
    ProcessingResult result;
    result.epoch = epoch;

    if (block.isEmpty() || sampleRate <= 0) {
        emit blockProcessed(result);
        return;
    }

    if (epoch != lastEpoch || miHistorySampleRate != sampleRate || miChannelHistory.size() != block.size()) {
        miChannelHistory = QVector<QVector<double>>(block.size());
        m_occipitalHistory.clear();
        miHistorySampleRate = sampleRate;
        lastEpoch = epoch;
    }

    const int historySamples = qMax(sampleRate * 2, sampleRate);
    const int analysisWindowSamples = qMax(sampleRate, static_cast<int>(sampleRate * 1.5));
    // PSD 分析窗口：至少 1.5 秒数据以保证频率分辨率
    const int psdWindowSamples = qMax(sampleRate, static_cast<int>(sampleRate * 1.5));

    result.channelPowers.reserve(block.size());
    for (int ch = 0; ch < block.size(); ++ch) {
        QVector<double> &history = miChannelHistory[ch];
        history += block.at(ch);
        if (history.size() > historySamples) {
            history.remove(0, history.size() - historySamples);
        }

        QVector<double> analysisSegment = history;
        if (analysisSegment.size() > analysisWindowSamples) {
            analysisSegment = analysisSegment.mid(analysisSegment.size() - analysisWindowSamples);
        }

        const SignalProcessor::BandEnergy bandEnergy = signalProcessor.computeBandEnergy(analysisSegment, sampleRate);
        // MI热力图采用Alpha/Mu频段(8-13 Hz)
        result.channelPowers.append(bandEnergy.alpha);
    }

    // --- 工作线程预计算热力图（避免主线程重计算） ---
    if (!result.channelPowers.isEmpty() && m_channelPositions.size() >= result.channelPowers.size() * 2) {
        result.topomapImage = MneTopoMap::computeTopomap(
            m_channelPositions, result.channelPowers,
            m_topoGridSize, m_topoValueRange);
    }

    QVector<QVector<double>> alignedSignals;
    QVector<int> alignedChannelIndices;
    alignedSignals.reserve(occipitalChannels.size());
    alignedChannelIndices.reserve(occipitalChannels.size());

    for (int i = 0; i < occipitalChannels.size(); ++i) {
        const int ch = occipitalChannels.at(i);
        if (ch >= 0 && ch < block.size()) {
            alignedSignals.append(block.at(ch));
            alignedChannelIndices.append(ch);
        }
    }

    if (alignedSignals.isEmpty()) {
        result.hasVisualData = !result.channelPowers.isEmpty();
        emit blockProcessed(result);
        return;
    }

    QVector<double> occipitalSignal;
    const int sampleCount = alignedSignals.first().size();
    occipitalSignal.reserve(sampleCount);

    for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
        double sum = 0.0;
        int validCount = 0;

        for (const QVector<double> &signal : alignedSignals) {
            if (sampleIndex < signal.size()) {
                sum += signal.at(sampleIndex);
                ++validCount;
            }
        }

        if (validCount > 0) {
            occipitalSignal.append(sum / validCount);
        }
    }

    // 将当前帧枕叶平均信号累积到历史缓冲
    if (!occipitalSignal.isEmpty()) {
        m_occipitalHistory += occipitalSignal;
        const int maxOccHistory = qMax(psdWindowSamples, sampleRate * 3);
        if (m_occipitalHistory.size() > maxOccHistory) {
            m_occipitalHistory.remove(0, m_occipitalHistory.size() - maxOccHistory);
        }
        // PSD 使用累积的历史数据（≥1.5 秒窗口），保证频率分辨率
        QVector<double> psdSegment = m_occipitalHistory;
        if (psdSegment.size() > psdWindowSamples) {
            psdSegment = psdSegment.mid(psdSegment.size() - psdWindowSamples);
        }
        result.psd = signalProcessor.computePSD(psdSegment, sampleRate);

        // 同步 FBCCA 刺激频率（与当前数据一致）
        m_fbcca.setStimulusFreqs(leftStimulusFreq, rightStimulusFreq);

        // FBCCA 算法：对多通道枕区信号做滤波器组典型相关分析
        // 保留原始多通道信息，不平均（FBCCA 利用多通道相关性提高信噪比）
        if (alignedSignals.size() >= 2) {
            // 如果样本不足一个完整周期，使用最近累积的历史数据
            int fbccaSamples = alignedSignals.first().size();
            for (int i = 1; i < alignedSignals.size(); ++i) {
                fbccaSamples = qMin(fbccaSamples, alignedSignals.at(i).size());
            }
            const int minFbccaSamples = sampleRate / 2; // 至少 0.5 秒
            if (fbccaSamples < minFbccaSamples) {
                // 使用历史缓冲中累积的数据
                QVector<QVector<double>> fbccaInput(alignedSignals.size());
                for (int i = 0; i < alignedSignals.size(); ++i) {
                    const int ch = alignedChannelIndices.at(i);
                    if (ch >= 0 && ch < miChannelHistory.size()) {
                        const auto &hist = miChannelHistory[ch];
                        int take = qMin(hist.size(), minFbccaSamples);
                        fbccaInput[i] = hist.mid(hist.size() - take);
                    }
                }
                result.fbcca = m_fbcca.process(fbccaInput, sampleRate);
            } else {
                result.fbcca = m_fbcca.process(alignedSignals, sampleRate);
            }
        } else if (alignedSignals.size() == 1) {
            // 单通道时复制一份以满足多通道要求
            QVector<QVector<double>> dup = alignedSignals;
            dup.append(alignedSignals[0]);
            result.fbcca = m_fbcca.process(dup, sampleRate);
        }
    }

    result.hasVisualData = true;
    emit blockProcessed(result);
}
