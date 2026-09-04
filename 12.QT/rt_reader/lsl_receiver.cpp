// ============================================================================
// lsl_receiver.cpp —— 见 lsl_receiver.h
// ============================================================================
#include "lsl_receiver.h"

#include "include/lsl_cpp.h"

#include <chrono>
#include <vector>

LslReceiver::LslReceiver(QObject *parent) : QThread(parent) {}

void LslReceiver::configure(const QString &type, const QString &name,
                            double resolveTimeout) {
    type_ = type;
    name_ = name;
    resolveTimeout_ = resolveTimeout;
}

void LslReceiver::stop() { running_ = false; }

void LslReceiver::run() {
    running_ = true;
    const auto tStart = std::chrono::steady_clock::now();

    // ---- 1) resolve:UDP 探测,寻找匹配的流 ----
    emit logMessage(QStringLiteral("resolve: 寻找 type=%1 ...").arg(type_));
    std::vector<lsl::stream_info> streams;
    try {
        if (name_.isEmpty())
            streams = lsl::resolve_stream("type", type_.toStdString(),
                                          1, resolveTimeout_);
        else
            streams = lsl::resolve_stream("name", name_.toStdString(),
                                          1, resolveTimeout_);
    } catch (const std::exception &e) {
        emit errorOccurred(QStringLiteral("resolve 异常: %1").arg(e.what()));
        return;
    }
    if (streams.empty()) {
        emit errorOccurred(QStringLiteral(
            "没有发现匹配的流(type=%1)。\n请先运行模拟器:\n"
            "  ./bin/lsl_sim --file data/cwz_04151049.bdf --loop")
                               .arg(type_));
        return;
    }

    lsl::stream_info info = streams[0];
    const int nch = info.channel_count();
    const double fs = info.nominal_srate();
    emit streamFound(QString::fromStdString(info.name()),
                     QString::fromStdString(info.type()), nch, fs);
    emit logMessage(QStringLiteral("打开 inlet: %1 | ch=%2 | %3 Hz")
                        .arg(QString::fromStdString(info.name()))
                        .arg(nch)
                        .arg(fs));

    // ---- 2) 建 inlet 并打开 TCP 数据连接 ----
    lsl::stream_inlet inlet(info);
    try {
        inlet.open_stream(5.0);
    } catch (const std::exception &e) {
        emit errorOccurred(QStringLiteral("open_stream 失败: %1").arg(e.what()));
        return;
    }
    emit logMessage(QStringLiteral("开始拉取..."));

    // ---- 3) 拉取循环 ----
    const std::size_t kMaxSamp = 4096;  // 每轮最多样本数
    std::vector<float> buf(static_cast<std::size_t>(nch) * kMaxSamp);
    std::vector<double> tss(kMaxSamp);
    const std::size_t dataElems = buf.size();
    // 注: v1.17 高性能重载 5 参语义见仓库记忆/rt_reader.cpp 注释

    long long totalSamples = 0;
    long long winSamples = 0;
    auto winStart = std::chrono::steady_clock::now();

    while (running_.load()) {
        std::size_t nElem = 0;
        try {
            nElem = inlet.pull_chunk_multiplexed(buf.data(), tss.data(),
                                                 dataElems, kMaxSamp, 0.2);
        } catch (const std::exception &e) {
            emit errorOccurred(QStringLiteral("拉取异常(链路断开?): %1")
                                   .arg(e.what()));
            break;
        }
        const std::size_t n = nElem / static_cast<std::size_t>(nch);
        if (n > 0) {
            // float -> double,发射整批(交错排列)
            QVector<double> mux;
            mux.reserve(static_cast<int>(n) * nch);
            const std::size_t total =
                n * static_cast<std::size_t>(nch);
            for (std::size_t i = 0; i < total; ++i)
                mux.append(static_cast<double>(buf[i]));
            emit samplesBatch(mux, static_cast<int>(n));
            totalSamples += static_cast<long long>(n);
            winSamples += static_cast<long long>(n);
        }

        // 每秒统计
        const auto now = std::chrono::steady_clock::now();
        const double ws =
            std::chrono::duration<double>(now - winStart).count();
        if (ws >= 1.0) {
            const double rate = static_cast<double>(winSamples) / ws;
            const double runSec = std::chrono::duration<double>(now - tStart).count();
            emit statsUpdated(rate, runSec, totalSamples);
            winSamples = 0;
            winStart = now;
        }
    }
    emit logMessage(QStringLiteral("接收线程已退出(总 %1 样本)")
                        .arg(totalSamples));
}
