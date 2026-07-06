// signalprocessor.cpp
#include "signalprocessor.h"
#include <cmath>
#include <QDebug>

SignalProcessor::SignalProcessor(QObject *parent) : QObject(parent) {
    fftSize = 1024;
    in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * fftSize);
    out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * fftSize);
    plan = fftw_plan_dft_1d(fftSize, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
}

SignalProcessor::~SignalProcessor() {
    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);
}

QVector<double> SignalProcessor::computePSD(const QVector<double> &signal, double sampleRate) {
    if (sampleRate <= 0.0) {
        return {};
    }

    const int n = qMin(signal.size(), fftSize);
    if (n < 2) {
        return {};
    }

    // 加汉宁窗
    for (int i = 0; i < n; i++) {
        const double window = 0.5 * (1 - cos(2 * M_PI * i / (n - 1)));
        in[i][0] = signal[i] * window;
        in[i][1] = 0;
    }
    for (int i = n; i < fftSize; i++) {
        in[i][0] = 0;
        in[i][1] = 0;
    }

    // FFT
    fftw_execute(plan);

    // 计算功率谱
    QVector<double> psd(fftSize / 2);
    for (int i = 0; i < fftSize / 2; i++) {
        double magnitude = sqrt(out[i][0]*out[i][0] + out[i][1]*out[i][1]);
        psd[i] = 20 * log10(magnitude + 1e-10); // dB
    }

    return psd;
}

SignalProcessor::BandEnergy SignalProcessor::computeBandEnergy(const QVector<double> &signal, double sampleRate)
{
    const QVector<double> psd = computePSD(signal, sampleRate);
    BandEnergy energy{0.0, 0.0, 0.0, 0.0, 0.0};

    if (psd.isEmpty() || sampleRate <= 0.0) {
        return energy;
    }

    const double binWidth = sampleRate / (psd.size() * 2.0);
    int deltaCount = 0;
    int thetaCount = 0;
    int alphaCount = 0;
    int betaCount = 0;
    int gammaCount = 0;

    for (int index = 0; index < psd.size(); ++index) {
        const double freq = index * binWidth;
        // 先转回线性功率再做频带积分，避免直接累加dB产生偏差
        const double linearPower = std::pow(10.0, psd[index] / 10.0);

        if (freq >= 0.5 && freq < 4.0) {
            energy.delta += linearPower;
            ++deltaCount;
        } else if (freq < 8.0) {
            energy.theta += linearPower;
            ++thetaCount;
        } else if (freq < 13.0) {
            energy.alpha += linearPower;
            ++alphaCount;
        } else if (freq < 30.0) {
            energy.beta += linearPower;
            ++betaCount;
        } else if (freq <= 50.0) {
            energy.gamma += linearPower;
            ++gammaCount;
        }
    }

    const auto toDb = [](double sumPower, int count) {
        if (count <= 0) {
            return 0.0;
        }
        const double meanPower = sumPower / static_cast<double>(count);
        return 10.0 * std::log10(meanPower + 1e-12);
    };

    energy.delta = toDb(energy.delta, deltaCount);
    energy.theta = toDb(energy.theta, thetaCount);
    energy.alpha = toDb(energy.alpha, alphaCount);
    energy.beta = toDb(energy.beta, betaCount);
    energy.gamma = toDb(energy.gamma, gammaCount);

    return energy;
}
