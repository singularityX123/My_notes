#ifndef CHARTMANAGER_H
#define CHARTMANAGER_H

#include <QObject>
#include <QVector>
#include <QPointF>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChartView>

#include <QLabel>

struct FbccaResult;

// ============================================================================
// ChartManager — 负责所有 QChart + SSVEP 状态栏的创建、配置和数据更新
// 单一责任：可视化层管理（频谱图 + 脑电波形图 + SSVEP检测状态）
// ============================================================================

class ChartManager : public QObject
{
    Q_OBJECT

public:
    explicit ChartManager(QObject *parent = nullptr);

    // 初始化图表并放置到指定布局
    void initSpectrumChart(QLayout *layout);
    void initEEGChart(QLayout *layout);

    // 数据更新
    void updateSpectrum(const QVector<double> &psd, double sampleRate);
    void updateEEGWaveform(const QVector<double> &buffer, int sampleRate);

    // SSVEP 状态栏更新
    void updateSSVEPUI(QLabel *leftStatus, QLabel *rightStatus,
                       QLabel *directionLabel,
                       const FbccaResult &fbcca,
                       double leftFreq, double rightFreq);

    // 清空 SSVEP UI（非想象期调用）
    void clearSSVEPUI(QLabel *leftStatus, QLabel *rightStatus,
                      QLabel *directionLabel);

    // 清空图表数据
    void clearSpectrum();
    void clearEEG();

    // 访问器（供外部调整轴范围等）
    QChartView *spectrumChartView() const { return m_spectrumChartView; }
    QChartView *eegChartView()       const { return m_eegChartView; }

private:
    QLineSeries *m_spectrumSeries = nullptr;
    QChartView  *m_spectrumChartView = nullptr;

    QLineSeries *m_eegSeries = nullptr;
    QChartView  *m_eegChartView = nullptr;
};

#endif // CHARTMANAGER_H
