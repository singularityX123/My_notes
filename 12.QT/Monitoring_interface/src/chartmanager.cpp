#include "chartmanager.h"
#include "ssvepfbcca.h"
#include <QtCharts/QChart>
#include <QtCharts/QValueAxis>
#include <QLayout>
#include <QFont>
//#include <algorithm>
#include <limits>

ChartManager::ChartManager(QObject *parent)
    : QObject(parent)
{
}

void ChartManager::initSpectrumChart(QLayout *layout)
{
    // --- 频谱图 ---
    auto *chart = new QChart();
    chart->setTitle("枕叶区域功率谱密度 (PSD)");
    chart->setAnimationOptions(QChart::NoAnimation);

    m_spectrumSeries = new QLineSeries();
    m_spectrumSeries->setName("PSD");
    chart->addSeries(m_spectrumSeries);

    auto *axisX = new QValueAxis();
    axisX->setTitleText("频率 (Hz)");
    axisX->setTitleFont(QFont("Sans Serif", 9));
    axisX->setRange(8, 20);
    axisX->setTickCount(25);
    axisX->setLabelFormat("%.1f");

    auto *axisY = new QValueAxis();
    axisY->setTitleText("功率谱密度 (dB)");
    axisY->setTitleFont(QFont("Sans Serif", 9));
    axisY->setRange(-60, 30);
    axisY->setLabelFormat("%.0f");

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    m_spectrumSeries->attachAxis(axisX);
    m_spectrumSeries->attachAxis(axisY);

    m_spectrumChartView = new QChartView(chart);
    m_spectrumChartView->setRenderHint(QPainter::Antialiasing);
    m_spectrumChartView->setMinimumHeight(450);

    if (layout)
        layout->addWidget(m_spectrumChartView);
}

void ChartManager::initEEGChart(QLayout *layout)
{
    // --- 脑电波形图 ---
    auto *chart = new QChart();
    chart->setTitle("实时脑电波形 (Cz通道)");
    chart->setAnimationOptions(QChart::NoAnimation);

    m_eegSeries = new QLineSeries();
    m_eegSeries->setName("Cz");
    chart->addSeries(m_eegSeries);

    auto *axisX = new QValueAxis();
    axisX->setTitleText("时间 (s)");
    axisX->setTitleFont(QFont("Sans Serif", 9));
    axisX->setRange(0.0, 4.0);
    axisX->setLabelFormat("%.1f");

    auto *axisY = new QValueAxis();
    axisY->setTitleText("幅值");
    axisY->setTitleFont(QFont("Sans Serif", 9));
    axisY->setRange(-150.0, 150.0);
    axisY->setLabelFormat("%.0f");

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    m_eegSeries->attachAxis(axisX);
    m_eegSeries->attachAxis(axisY);

    m_eegChartView = new QChartView(chart);
    m_eegChartView->setRenderHint(QPainter::Antialiasing);
    m_eegChartView->setMinimumHeight(280);

    if (layout)
        layout->addWidget(m_eegChartView);
}

void ChartManager::updateSpectrum(const QVector<double> &psd, double sampleRate)
{
    if (!m_spectrumSeries || !m_spectrumChartView)
        return;

    QChart *chart = m_spectrumChartView->chart();
    if (!chart)
        return;

    m_spectrumSeries->clear();

    const int maxIndex = qMin(psd.size(), static_cast<int>(sampleRate / 2.0));
    if (maxIndex <= 0)
        return;

    QVector<QPointF> points;
    points.reserve(maxIndex);
    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::lowest();
    for (int i = 0; i < maxIndex; ++i) {
        // Δf = sampleRate / N, psd.size() = N/2 => Δf = sampleRate / (psd.size() * 2)
        const double freq  = i * sampleRate / qMax(1, psd.size() * 2);
        const double value = psd.at(i);
        points.append(QPointF(freq, value));
        minY = qMin(minY, value);
        maxY = qMax(maxY, value);
    }
    m_spectrumSeries->replace(points);

    // Y轴自适应
    const auto axesY = chart->axes(Qt::Vertical);
    if (!axesY.isEmpty()) {
        if (auto *axisY = qobject_cast<QValueAxis*>(axesY.first())) {
            if (std::abs(maxY - minY) < 1e-6) {
                maxY += 1.0; minY -= 1.0;
            }
            const double margin = (maxY - minY) * 0.05;
            axisY->setRange(minY - margin, maxY + margin);
        }
    }

    // X轴固定 9-15 Hz
    const auto axesX = chart->axes(Qt::Horizontal);
    if (!axesX.isEmpty()) {
        if (auto *axisX = qobject_cast<QValueAxis*>(axesX.first()))
            axisX->setRange(9, 15);
    }
}

void ChartManager::updateEEGWaveform(const QVector<double> &buffer, int sampleRate)
{
    if (!m_eegSeries || !m_eegChartView || sampleRate <= 0)
        return;

    QChart *chart = m_eegChartView->chart();
    if (!chart)
        return;

    const int count = buffer.size();
    if (count <= 0)
        return;

    QVector<QPointF> points;
    points.reserve(count);
    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::lowest();
    for (int i = 0; i < count; ++i) {
        const double y = buffer.at(i);
        const double x = static_cast<double>(i) / sampleRate;
        points.append(QPointF(x, y));
        minY = qMin(minY, y);
        maxY = qMax(maxY, y);
    }
    m_eegSeries->replace(points);

    const auto axesX = chart->axes(Qt::Horizontal);
    if (!axesX.isEmpty()) {
        if (auto *axisX = qobject_cast<QValueAxis*>(axesX.first()))
            axisX->setRange(0.0, qMax(0.5, static_cast<double>(count) / sampleRate));
    }

    const auto axesY = chart->axes(Qt::Vertical);
    if (!axesY.isEmpty()) {
        if (auto *axisY = qobject_cast<QValueAxis*>(axesY.first())) {
            if (std::abs(maxY - minY) < 1e-6) {
                maxY += 1.0; minY -= 1.0;
            }
            const double margin = (maxY - minY) * 0.2;
            axisY->setRange(minY - margin, maxY + margin);
        }
    }
}

void ChartManager::clearSpectrum()
{
    if (m_spectrumSeries)
        m_spectrumSeries->clear();
}

void ChartManager::clearEEG()
{
    if (m_eegSeries)
        m_eegSeries->clear();
}

void ChartManager::updateSSVEPUI(QLabel *leftStatus, QLabel *rightStatus,
                                  QLabel *directionLabel,
                                  const FbccaResult &fbcca,
                                  double leftFreq, double rightFreq)
{
    // 方向判别
    QString dirText, dirStyle;
    if (fbcca.direction > 0.5) {
        dirText  = "注视方向  ->";
        dirStyle = "color: orange; font-weight: bold;";
    } else if (fbcca.direction < -0.5) {
        dirText  = "注视方向  <-";
        dirStyle = "color: #1ab2c6; font-weight: bold;";
    } else {
        dirText  = "注视方向  不确定";
        dirStyle = "color: gray; font-weight: bold;";
    }
    if (directionLabel) {
        directionLabel->setText(dirText);
        directionLabel->setStyleSheet(dirStyle);
    }

    // 状态标签
    if (leftStatus){
        leftStatus->setText(QString("左侧视野: %1 Hz\nFBCCA 概率: %2%\n相关得分: %3")
            .arg(leftFreq, 0, 'f', 1)
            .arg(fbcca.leftProb * 100.0, 0, 'f', 1)
            .arg(fbcca.leftScore, 0, 'f', 3));
        leftStatus->setAlignment(Qt::AlignCenter);
        
        QFont font = leftStatus->font();
        font.setPointSize(13); // 设置字号为 13
        //font.setBold(true);    // 可选：加粗
        leftStatus->setFont(font);
    }
    if (rightStatus){
        rightStatus->setText(QString("右侧视野: %1 Hz\nFBCCA 概率: %2%\n相关得分: %3")
            .arg(rightFreq, 0, 'f', 1)
            .arg(fbcca.rightProb * 100.0, 0, 'f', 1)
            .arg(fbcca.rightScore, 0, 'f', 3));
        rightStatus->setAlignment(Qt::AlignCenter);
        
        QFont font = rightStatus->font();
        font.setPointSize(13);
        //font.setBold(true);    // 可选：加粗
        rightStatus->setFont(font);
    }
}

void ChartManager::clearSSVEPUI(QLabel *leftStatus, QLabel *rightStatus,
                                 QLabel *directionLabel)
{
    if (leftStatus)  leftStatus->clear();
    if (rightStatus) rightStatus->clear();
    if (directionLabel) {
        directionLabel->clear();
    }
}
