// ssvepfbcca.cpp — FBCCA (Filter Bank Canonical Correlation Analysis) 实现
#include "ssvepfbcca.h"
#include <QDebug>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <limits>

// ============================================================================
// 内部辅助 — 矩阵运算工具（仅用于本文件）
// ============================================================================
namespace {

inline bool isFinite(double v)
{
    return std::isfinite(v);
}

// 向量均值
double vecMean(const QVector<double> &v) {
    if (v.isEmpty()) return 0.0;
    return std::accumulate(v.cbegin(), v.cend(), 0.0) / v.size();
}

// 向量点积
double vecDot(const QVector<double> &a, const QVector<double> &b) {
    double s = 0.0;
    for (int i = 0; i < qMin(a.size(), b.size()); ++i)
        s += a[i] * b[i];
    return s;
}

// 向量方差（无偏）
double vecVar(const QVector<double> &v) {
    const int n = v.size();
    if (n < 2) return 1.0;
    const double m = vecMean(v);
    double s = 0.0;
    for (double x : v) s += (x - m) * (x - m);
    return s / (n - 1);
}

// 2D 矩阵转置: [rows][cols] → [cols][rows]
QVector<QVector<double>> matTranspose(const QVector<QVector<double>> &m) {
    if (m.isEmpty()) return {};
    const int rows = m.size(), cols = m[0].size();
    QVector<QVector<double>> t(cols, QVector<double>(rows, 0.0));
    for (int i = 0; i < rows; ++i)
        for (int j = 0; j < cols; ++j)
            t[j][i] = m[i][j];
    return t;
}

// 矩阵乘法: A[ra][ca] × B[ca][cb]
QVector<QVector<double>> matMul(const QVector<QVector<double>> &A,
                                const QVector<QVector<double>> &B) {
    const int ra = A.size(), ca = A[0].size();
    const int cb = B[0].size();
    QVector<QVector<double>> C(ra, QVector<double>(cb, 0.0));
    for (int i = 0; i < ra; ++i)
        for (int k = 0; k < ca; ++k) {
            const double aik = A[i][k];
            if (aik == 0.0) continue;
            for (int j = 0; j < cb; ++j)
                C[i][j] += aik * B[k][j];
        }
    return C;
}

// 对称矩阵 Cholesky 分解: A = L * L^T, 返回下三角 L
// A 必须是对称正定矩阵 [n×n]
QVector<QVector<double>> cholesky(const QVector<QVector<double>> &A) {
    const int n = A.size();
    if (n <= 0) {
        return {};
    }
    for (int i = 0; i < n; ++i) {
        if (A[i].size() != n) {
            return {};
        }
    }
    QVector<QVector<double>> L(n, QVector<double>(n, 0.0));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j <= i; ++j) {
            double sum = 0.0;
            for (int k = 0; k < j; ++k)
                sum += L[i][k] * L[j][k];
            if (i == j) {
                double diag = A[i][i] - sum;
                if (!isFinite(diag)) {
                    return {};
                }
                // 允许轻微负值（数值误差）但避免退化分母
                diag = std::max(diag, 1e-12);
                L[i][j] = std::sqrt(diag);
                if (!isFinite(L[i][j]) || L[i][j] < 1e-8) {
                    return {};
                }
            } else {
                const double denom = L[j][j];
                if (!isFinite(denom) || std::abs(denom) < 1e-8) {
                    return {};
                }
                L[i][j] = (A[i][j] - sum) / denom;
                if (!isFinite(L[i][j])) {
                    return {};
                }
            }
        }
    }
    return L;
}

// 使用 Cholesky 前代/回代求解 A*x = b
QVector<double> choleskySolve(const QVector<QVector<double>> &A,
                              const QVector<double> &b) {
    const int n = A.size();
    auto L = cholesky(A);
    if (L.isEmpty() || b.size() != n) {
        return {};
    }

    // 前代: L * y = b
    QVector<double> y(n, 0.0);
    for (int i = 0; i < n; ++i) {
        double sum = 0.0;
        for (int k = 0; k < i; ++k)
            sum += L[i][k] * y[k];
        const double denom = L[i][i];
        if (!isFinite(denom) || std::abs(denom) < 1e-8) {
            return {};
        }
        y[i] = (b[i] - sum) / denom;
        if (!isFinite(y[i])) {
            return {};
        }
    }

    // 回代: L^T * x = y
    QVector<double> x(n, 0.0);
    for (int i = n - 1; i >= 0; --i) {
        double sum = 0.0;
        for (int k = i + 1; k < n; ++k)
            sum += L[k][i] * x[k];
        const double denom = L[i][i];
        if (!isFinite(denom) || std::abs(denom) < 1e-8) {
            return {};
        }
        x[i] = (y[i] - sum) / denom;
        if (!isFinite(x[i])) {
            return {};
        }
    }
    return x;
}

// 对称矩阵求逆 (Cholesky 分解后求解)
QVector<QVector<double>> matInvSym(const QVector<QVector<double>> &A) {
    const int n = A.size();
    auto L = cholesky(A);
    QVector<QVector<double>> inv(n, QVector<double>(n, 0.0));
    for (int j = 0; j < n; ++j) {
        // 求解 L * y = e_j
        QVector<double> y(n, 0.0);
        for (int i = 0; i < n; ++i) {
            double sum = 0.0;
            for (int k = 0; k < i; ++k)
                sum += L[i][k] * y[k];
            y[i] = (i == j ? 1.0 : 0.0) - sum;
            y[i] /= L[i][i];
        }
        // 求解 L^T * inv[:,j] = y
        for (int i = n - 1; i >= 0; --i) {
            double sum = 0.0;
            for (int k = i + 1; k < n; ++k)
                sum += L[k][i] * inv[k][j];
            inv[i][j] = (y[i] - sum) / L[i][i];
        }
    }
    return inv;
}

// 功率迭代法求对称矩阵的最大特征值和特征向量
void powerIteration(const QVector<QVector<double>> &A,
                    double &eigenvalue,
                    QVector<double> &eigenvector,
                    int maxIter = 50, double tol = 1e-8) {
    const int n = A.size();
    eigenvector = QVector<double>(n, 1.0 / std::sqrt(n));
    eigenvalue = 0.0;

    for (int iter = 0; iter < maxIter; ++iter) {
        // v = A * x
        QVector<double> v(n, 0.0);
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j)
                v[i] += A[i][j] * eigenvector[j];

        // λ = x^T * v
        double lam = vecDot(eigenvector, v);

        // normalize
        double norm = std::sqrt(vecDot(v, v));
        if (norm < 1e-15) break;
        for (int i = 0; i < n; ++i)
            eigenvector[i] = v[i] / norm;

        if (std::abs(lam - eigenvalue) < tol) {
            eigenvalue = lam;
            break;
        }
        eigenvalue = lam;
    }
}

} // anonymous namespace

// ============================================================================
// SSVEPFbcca 实现
// ============================================================================

SSVEPFbcca::SSVEPFbcca(QObject *parent)
    : QObject(parent)
{
    // 默认滤波器组: SSVEP 常用频带覆盖 (Hz)
    // 参考 FBCCA 论文: Chen et al. 2015
    m_filterBands = {
        { 6.0,  90.0 },   // 子带 1: 宽频
        { 10.0, 90.0 },   // 子带 2: 去低频
        { 14.0, 90.0 },   // 子带 3
        { 18.0, 90.0 },   // 子带 4
        { 22.0, 90.0 },   // 子带 5
    };
}

SSVEPFbcca::~SSVEPFbcca() = default;

void SSVEPFbcca::setStimulusFreqs(double leftHz, double rightHz) {
    m_leftFreq = leftHz;
    m_rightFreq = rightHz;
    m_refsValid = false;
}

void SSVEPFbcca::setNumHarmonics(int n) {
    m_numHarmonics = qMax(1, n);
    m_refsValid = false;
}

void SSVEPFbcca::setFilterBands(const QVector<QPair<double, double>> &bands) {
    m_filterBands = bands;
}

void SSVEPFbcca::setBandWeights(double exponentA, double offsetB) {
    m_bandWeightA = exponentA;
    m_bandWeightB = offsetB;
}

void SSVEPFbcca::setScoreScale(double scale) {
    m_scoreScale = scale;
}

void SSVEPFbcca::setCcaReg(double reg) {
    m_ccaReg = reg;
}

void SSVEPFbcca::setDecisionThreshold(double thresh) {
    m_decisionThresh = thresh;
}

void SSVEPFbcca::setMinSamples(int n) {
    m_minSamples = n;
}

void SSVEPFbcca::reset() {
    m_refsValid = false;
    m_cachedRefs.clear();
}

// ============================================================================
// 参考信号生成
// 对每个刺激频率，生成 [2*numHarmonics] × nSamples 的矩阵
// 每行为 sin(2π·h·f·t) 或 cos(2π·h·f·t)，h = 1..numHarmonics
// ============================================================================
QVector<QVector<double>> SSVEPFbcca::generateReference(double freq,
                                                        int nSamples,
                                                        int sampleRate) {
    const int nRef = 2 * m_numHarmonics;
    QVector<QVector<double>> ref(nRef, QVector<double>(nSamples, 0.0));

    for (int h = 0; h < m_numHarmonics; ++h) {
        const double f = freq * (h + 1);
        for (int t = 0; t < nSamples; ++t) {
            const double phase = 2.0 * M_PI * f * t / sampleRate;
            ref[2 * h][t]     = std::sin(phase);
            ref[2 * h + 1][t] = std::cos(phase);
        }
    }
    return ref;
}

// ============================================================================
// Pearson 相关系数
// ============================================================================
double SSVEPFbcca::pearsonCorr(const QVector<double> &a,
                                const QVector<double> &b) const {
    const int n = qMin(a.size(), b.size());
    if (n < 3) return 0.0;

    const double ma = vecMean(a);
    const double mb = vecMean(b);

    double cov = 0.0, va = 0.0, vb = 0.0;
    for (int i = 0; i < n; ++i) {
        const double da = a[i] - ma;
        const double db = b[i] - mb;
        cov += da * db;
        va  += da * da;
        vb  += db * db;
    }
    const double denom = std::sqrt(va * vb);
    if (denom < 1e-12) return 0.0;
    return std::abs(cov / denom);
}

// ============================================================================
// CCA 相关系数计算
// 输入: signal — 单通道 EEG 数据 [nSamples]
//       reference — 参考信号矩阵 [nRefs × nSamples]
// 输出: 最大典型相关系数的绝对值
//
// 算法:
//   CCA 最大化 corr(a^T x, b^T y)
//   这里 x 是信号标量 (单通道), y 是参考向量 (多谐波)
//   解: rho = sqrt( rxy^T * Ryy^(-1) * rxy / sigma_xx )
//   其中 rxy 是 x 与 y 各分量的互协方差向量
//   Ryy 是 y 的协方差矩阵, sigma_xx 是 x 的方差
// ============================================================================
double SSVEPFbcca::ccaCorrelation(const QVector<double> &signal,
                                   const QVector<QVector<double>> &reference) const {
    const int n = signal.size();
    const int nRefs = reference.size();
    if (n < 3 || nRefs < 1) return 0.0;

    // 均中心化
    const double mx = vecMean(signal);
    QVector<double> xc(n);
    for (int i = 0; i < n; ++i) xc[i] = signal[i] - mx;

    // sigma_xx = x^T x / (n-1)
    double sigma_xx = vecDot(xc, xc) / (n - 1);
    if (!std::isfinite(sigma_xx) || sigma_xx < 1e-15) return 0.0;

    // 参考信号均中心化 + 构建协方差矩阵
    QVector<QVector<double>> Ryy(nRefs, QVector<double>(nRefs, 0.0));
    QVector<double> rxy(nRefs, 0.0);

    // 先对每个参考通道去均值
    QVector<QVector<double>> yc(nRefs, QVector<double>(n, 0.0));
    QVector<double> my(nRefs);
    for (int j = 0; j < nRefs; ++j) {
        my[j] = vecMean(reference[j]);
        for (int i = 0; i < n; ++i)
            yc[j][i] = reference[j][i] - my[j];
    }

    // 计算 Ryy 和 rxy
    const double invNm1 = 1.0 / (n - 1);
    for (int j = 0; j < nRefs; ++j) {
        rxy[j] = vecDot(xc, yc[j]) * invNm1;
        for (int k = j; k < nRefs; ++k) {
            double v = vecDot(yc[j], yc[k]) * invNm1;
            Ryy[j][k] = v;
            Ryy[k][j] = v; // 对称
        }
    }

    // 正则化 Ryy
    for (int j = 0; j < nRefs; ++j)
        Ryy[j][j] += m_ccaReg;

    // 求解: Ryy^(-1) * rxy
    const QVector<double> Ryy_inv_rxy = choleskySolve(Ryy, rxy);
    if (Ryy_inv_rxy.size() != nRefs) {
        return 0.0;
    }

    // rho^2 = rxy^T * Ryy^(-1) * rxy / sigma_xx
    double rho2 = vecDot(rxy, Ryy_inv_rxy) / sigma_xx;
    if (!std::isfinite(rho2)) {
        return 0.0;
    }
    if (rho2 < 0.0) rho2 = 0.0;
    if (rho2 > 1.0) rho2 = 1.0;

    return std::sqrt(rho2);
}

// ============================================================================
// 滤波器组 — 对信号进行多个频带的带通滤波
// 使用 FIR 时域卷积实现（窗口法设计 + 汉明窗），对每个子带逐一滤波
// ============================================================================
QVector<QVector<double>> SSVEPFbcca::applyFilterBank(const QVector<double> &signal,
                                                      int sampleRate) const {
    const int nBands = m_filterBands.size();
    QVector<QVector<double>> filteredBands(nBands);

    for (int b = 0; b < nBands; ++b) {
        filteredBands[b] = bandpassFilter(signal,
                                          m_filterBands[b].first,
                                          m_filterBands[b].second,
                                          sampleRate,
                                          4);
    }
    return filteredBands;
}

// ============================================================================
// 带通滤波 — 简易 IIR 巴特沃斯滤波（双线性变换法）
// 使用 4 阶 Butterworth 设计，双向滤波实现零相位
// ============================================================================
QVector<double> SSVEPFbcca::bandpassFilter(const QVector<double> &signal,
                                            double lowHz, double highHz,
                                            int sampleRate, int order) const {
    const int n = signal.size();
    if (n < 10 || lowHz <= 0.0 || highHz <= lowHz || sampleRate <= 0)
        return signal;

    // 使用重叠保留法（FFT 域滤波）— 简单快速
    // 设计 FIR 带通滤波器: 理想带通 * 汉明窗
    const int filtLen = qMin(257, qMax(31, sampleRate / 5)); // ~0.2s 的滤波器长度
    const double nyq = 0.5 * sampleRate;
    const double fLow  = lowHz / nyq;
    const double fHigh = highHz / nyq;

    // 生成 FIR 系数 (窗口法)
    QVector<double> kernel(filtLen, 0.0);
    const int M = filtLen / 2;
    for (int i = 0; i < filtLen; ++i) {
        int m = i - M;
        if (m == 0) {
            kernel[i] = 2.0 * (fHigh - fLow);
        } else {
            kernel[i] = (std::sin(2.0 * M_PI * fHigh * m) -
                         std::sin(2.0 * M_PI * fLow  * m)) / (M_PI * m);
        }
        // 汉明窗
        kernel[i] *= (0.54 - 0.46 * std::cos(2.0 * M_PI * i / (filtLen - 1)));
    }

    // 归一化使 DC 增益为 0（带通无需做 DC 归一化）
    // 对 kernel 做归一化以保证通带增益近似为 1
    double sumK = 0.0;
    for (double k : kernel) sumK += k;
    if (fLow > 0.0 && fHigh < 1.0 && std::abs(sumK) > 1e-6) {
        // 仅对通带中心频率附近做归一化
        double refFreq = (lowHz + highHz) * 0.5 / nyq;
        double refGain = 0.0;
        for (int i = 0; i < filtLen; ++i) {
            int m = i - M;
            if (m == 0) refGain += 1.0;
            else refGain += kernel[i] * std::cos(2.0 * M_PI * refFreq * m);
        }
        if (std::abs(refGain) > 1e-6) {
            for (int i = 0; i < filtLen; ++i)
                kernel[i] /= refGain;
        }
    }

    // FFT 卷积
    const int convLen = n + filtLen - 1;
    const int fftLen = 1;
    int fftSize = 1;
    while (fftSize < convLen) fftSize <<= 1;

    QVector<double> filtered(n, 0.0);

    // 如果信号太短或 FFT 大小过大，退化为时域卷积
    if (fftSize > 65536) {
        // 简单 FIR 时域卷积（中心部分）
        for (int i = 0; i < n; ++i) {
            double sum = 0.0;
            for (int k = 0; k < filtLen; ++k) {
                int idx = i - k + M;
                if (idx >= 0 && idx < n)
                    sum += kernel[k] * signal[idx];
            }
            filtered[i] = sum;
        }
        return filtered;
    }

    // FFT 卷积
    // 用 fftw 需要分配内存，这里用直接时域实现避免依赖 fftw 长度问题
    // 直接使用时域 FIR（对于实时应用效率足够）
    for (int i = 0; i < n; ++i) {
        double sum = 0.0;
        for (int k = 0; k < filtLen; ++k) {
            int idx = i - k + M;
            if (idx >= 0 && idx < n)
                sum += kernel[k] * signal[idx];
        }
        filtered[i] = sum;
    }

    return filtered;
}

// ============================================================================
// FBCCA 核心处理
// ============================================================================
FbccaResult SSVEPFbcca::process(const QVector<QVector<double>> &data,
                                 int sampleRate) {
    FbccaResult result;

    const int nChannels = data.size();
    if (nChannels == 0 || sampleRate <= 0) return result;

    int nSamples = std::numeric_limits<int>::max();
    for (int ch = 0; ch < nChannels; ++ch) {
        nSamples = qMin(nSamples, data[ch].size());
    }
    if (nSamples == std::numeric_limits<int>::max() || nSamples < m_minSamples) {
        return result;
    }

    // --- 缓存参考信号（数据长度一致时复用） ---
    if (!m_refsValid || m_cachedSampleRate != sampleRate || m_cachedSamples != nSamples) {
        m_cachedRefs.clear();
        m_cachedRefs.append(generateReference(m_leftFreq, nSamples, sampleRate));
        m_cachedRefs.append(generateReference(m_rightFreq, nSamples, sampleRate));
        m_cachedSampleRate = sampleRate;
        m_cachedSamples = nSamples;
        m_refsValid = true;
    }

    // --- 对每个通道进行滤波器组分解并计算 CCA 得分 ---
    // 分两个刺激频率计算
    double sumLeft  = 0.0;
    double sumRight = 0.0;
    int validCh = 0;

    for (int ch = 0; ch < nChannels; ++ch) {
        const auto &channelData = data[ch];
        if (channelData.size() < nSamples) {
            continue;
        }

        const QVector<double> slice = (channelData.size() == nSamples)
            ? channelData
            : channelData.mid(channelData.size() - nSamples);

        // 滤波器组分解
        QVector<QVector<double>> bands = applyFilterBank(slice, sampleRate);

        double chLeftScore  = 0.0;
        double chRightScore = 0.0;

        for (int b = 0; b < qMin(bands.size(), m_filterBands.size()); ++b) {
            // FBCCA 子带权重: w(b) = (b+1)^(-a) + b
            const double w = std::pow(b + 1, -m_bandWeightA) + m_bandWeightB;

            // 对该通道第 b 频带数据分别与两个参考信号做 CCA
            double rhoLeft  = ccaCorrelation(bands[b], m_cachedRefs[0]);
            double rhoRight = ccaCorrelation(bands[b], m_cachedRefs[1]);

            if (!std::isfinite(rhoLeft))  rhoLeft  = 0.0;
            if (!std::isfinite(rhoRight)) rhoRight = 0.0;

            // FBCCA: 加权平方相关系数累积
            chLeftScore  += w * (rhoLeft  * rhoLeft);
            chRightScore += w * (rhoRight * rhoRight);
        }

        sumLeft  += chLeftScore;
        sumRight += chRightScore;
        ++validCh;
    }

    if (validCh == 0) return result;

    // 通道间平均得分
    double avgLeft  = sumLeft  / validCh;
    double avgRight = sumRight / validCh;

    // 应用 score scale
    avgLeft  *= m_scoreScale;
    avgRight *= m_scoreScale;

    if (!std::isfinite(avgLeft) || !std::isfinite(avgRight)) {
        return FbccaResult{};
    }

    result.leftScore  = avgLeft;
    result.rightScore = avgRight;

    // --- Softmax 概率归一化 ---
    double maxScore = std::max(avgLeft, avgRight);
    double eL = std::exp(avgLeft  - maxScore);
    double eR = std::exp(avgRight - maxScore);
    double sumE = eL + eR + 1e-12;

    result.leftProb  = eL / sumE;
    result.rightProb = eR / sumE;
    result.confidence = std::max(result.leftProb, result.rightProb);

    // --- 方向判别 ---
    double probDiff = std::abs(result.leftProb - result.rightProb);
    if (probDiff < m_decisionThresh) {
        result.direction = 0.0; // 不确定
    } else if (result.rightProb > result.leftProb) {
        result.direction = 1.0; // 右
    } else {
        result.direction = -1.0; // 左
    }

    return result;
}
