// ssvepfbcca.h — FBCCA (Filter Bank Canonical Correlation Analysis) 算法实现
// 参考: traditional_mi_ssvep.py 中的 FBCCABranch 类
// 用于多通道 SSVEP 频率识别和注视方向判别
#ifndef SSVEPFBCCA_H
#define SSVEPFBCCA_H

#include <QObject>
#include <QVector>
#include <QPair>
#include <cmath>

// ============================================================================
// FbccaResult — FBCCA 单次处理输出
// ============================================================================
struct FbccaResult {
    double leftScore  = 0.0;   // 左刺激频率原始得分
    double rightScore = 0.0;   // 右刺激频率原始得分
    double leftProb   = 0.5;   // 左刺激概率（softmax 归一化后）
    double rightProb  = 0.5;   // 右刺激概率
    double direction  = 0.0;   // 判别方向: +1 右, -1 左, 0 不确定
    double confidence = 0.0;   // max(leftProb, rightProb)
};

// ============================================================================
// SSVEPFbcca — 滤波器组典型相关分析 SSVEP 检测器
// 职责：
//   1) 为每个刺激频率生成多谐波参考信号
//   2) 对多通道枕区 EEG 进行滤波器组分解
//   3) 计算各频带 CCA 相关系数并加权融合
//   4) softmax 概率归一化，输出注视方向判别
// ============================================================================
class SSVEPFbcca : public QObject {
    Q_OBJECT

public:
    explicit SSVEPFbcca(QObject *parent = nullptr);
    ~SSVEPFbcca() override;

    // --- 配置 ---
    // 设置左/右刺激频率 (Hz)
    void setStimulusFreqs(double leftHz, double rightHz);

    // 设置谐波数（默认 3: 基波 + 2 次 + 3 次）
    void setNumHarmonics(int n);

    // 设置滤波器组频带（每对为 [low, high] Hz），默认覆盖 SSVEP 常用范围
    void setFilterBands(const QVector<QPair<double, double>> &bands);

    // 设置滤波器组加权参数: w_b = (b+1)^(-a) + b （与 Python 版一致）
    void setBandWeights(double exponentA, double offsetB);

    // 设置 score 缩放因子
    void setScoreScale(double scale);

    // 设置 CCA 正则化参数
    void setCcaReg(double reg);

    // 设置判别阈值（概率差超过此值才判定方向，否则为不确定）
    void setDecisionThreshold(double thresh);

    // 设置最小有效样本数（数据不足时跳过处理）
    void setMinSamples(int n);

    // --- 处理 ---
    // 对多通道枕区 EEG 数据执行 FBCCA 检测
    // data: 每个 QVector<double> 为一个通道的时间序列
    // sampleRate: 采样率 (Hz)
    FbccaResult process(const QVector<QVector<double>> &data, int sampleRate);

    // 重置内部状态（切换数据源时调用）
    void reset();

private:
    // --- 参考信号生成 ---
    // 生成频率 freq 的参考信号矩阵: [2*nHarmonics] × nSamples
    // 每对为 sin/cos 谐波
    QVector<QVector<double>> generateReference(double freq, int nSamples, int sampleRate);

    // --- CCA 相关 ---
    // 计算两个信号间的 Pearson 相关系数
    double pearsonCorr(const QVector<double> &a, const QVector<double> &b) const;

    // 计算单通道信号与多谐波参考之间的 CCA 相关系数
    // 返回最大典型相关系数的绝对值
    double ccaCorrelation(const QVector<double> &signal,
                          const QVector<QVector<double>> &reference) const;

    // --- 滤波器组 ---
    // 对单通道信号应用滤波器组，返回每个频带的滤波结果
    QVector<QVector<double>> applyFilterBank(const QVector<double> &signal,
                                             int sampleRate) const;

    // --- 辅助: FIR 带通滤波 ---
    // 简单 IIR 巴特沃斯带通滤波（用于滤波器组分解）
    QVector<double> bandpassFilter(const QVector<double> &signal,
                                   double lowHz, double highHz,
                                   int sampleRate, int order = 4) const;

    // --- 参数 ---
    double m_leftFreq        = 10.0;
    double m_rightFreq       = 12.0;
    int    m_numHarmonics    = 3;
    double m_scoreScale      = 20.0;
    double m_ccaReg          = 1e-4;   // 正则化防止奇异性
    double m_decisionThresh  = 0.15;   // 概率差阈值
    int    m_minSamples      = 64;     // 最少采样点数

    // 滤波器组频带及权重参数（FBCCA 核心）
    QVector<QPair<double, double>> m_filterBands;
    double m_bandWeightA = 1.25;   // 指数衰减参数
    double m_bandWeightB = 0.25;   // 偏移参数

    // 最后一次使用的参考信号缓存（相同数据长度和采样率时复用）
    int    m_cachedSampleRate = 0;
    int    m_cachedSamples    = 0;
    QVector<QVector<QVector<double>>> m_cachedRefs; // [2 freqs][2*harmonics][samples]

    // 有效参考缓存
    bool   m_refsValid = false;
};

#endif // SSVEPFBCCA_H
