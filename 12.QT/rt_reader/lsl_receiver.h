// ============================================================================
// lsl_receiver.h —— LSL 拉流线程(QThread),供 GUI 与命令行共用
// ----------------------------------------------------------------------------
// 职责(对应链路):
//   run() 中执行 resolve(UDP 探测) -> stream_inlet -> open_stream(TCP 数据连接)
//   -> 循环 pull_chunk_multiplexed 拉取,把数据/统计用信号发回 GUI 线程。
// ============================================================================
#pragma once

#include <QThread>
#include <QVector>
#include <QString>

#include <atomic>
// 注: Qt6 已自动注册基础类型容器(如 QVector<double>),可直接作跨线程信号参数

class LslReceiver : public QThread {
    Q_OBJECT
public:
    explicit LslReceiver(QObject *parent = nullptr);

    // 连接前配置:type 过滤(如 "EEG"),name 为空表示取该类型第一个流
    void configure(const QString &type, const QString &name,
                   double resolveTimeout = 5.0);
    void stop();  // 请求线程优雅退出(线程安全,可从任意线程调用)

signals:
    // 成功解析到流(在子线程发射,自动排队到主线程槽)
    void streamFound(const QString &name, const QString &type,
                     int channels, double srate);
    // 一批"交错(multiplexed)"样本: 长度 = samples*channels
    // 位置 [sample*channels + ch] 即第 sample 个样本的第 ch 通道值
    void samplesBatch(const QVector<double> &mux, int samples);
    // 每秒一次的统计
    void statsUpdated(double rate, double totalSec, long long totalSamples);
    void errorOccurred(const QString &msg);
    void logMessage(const QString &msg);

protected:
    void run() override;

private:
    QString type_{"EEG"};
    QString name_;
    double resolveTimeout_{5.0};
    std::atomic<bool> running_{true};
};
