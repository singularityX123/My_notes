// signalprocessor.h
#ifndef SIGNALPROCESSOR_H
#define SIGNALPROCESSOR_H

#include <QObject>
#include <QVector>
#include <fftw3.h>

class SignalProcessor : public QObject {
    Q_OBJECT

public:
    explicit SignalProcessor(QObject *parent = nullptr);
    ~SignalProcessor();

    // 计算功率谱密度
    QVector<double> computePSD(const QVector<double> &signal, double sampleRate);

    // 计算时频图(频谱图)
    QVector<QVector<double>> computeSpectrogram(const QVector<double> &signal,
                                                double sampleRate,
                                                int windowSize,
                                                int overlap);

    // 计算脑电地形图插值
    QVector<QVector<double>> interpolateTopography(const QVector<double> &channelValues,
                                                   int gridSize = 64);

    // 提取频带能量
    struct BandEnergy {
        double delta;   // 0.5-4 Hz
        double theta;   // 4-8 Hz
        double alpha;   // 8-13 Hz
        double beta;    // 13-30 Hz
        double gamma;   // 30-50 Hz
    };
    BandEnergy computeBandEnergy(const QVector<double> &signal, double sampleRate);

private:
    fftw_plan plan;
    fftw_complex *in, *out;
    int fftSize;
};

#endif
