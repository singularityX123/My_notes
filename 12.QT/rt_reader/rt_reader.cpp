// ============================================================================
// rt_reader.cpp —— 命令行版 LSL 拉流端(先独立验证,后接入 Qt)
// ----------------------------------------------------------------------------
// 配合 lsl_data_transmission_simulator(.bdf 模拟推流)在局域网拉流：
//   UDP 探测 resolve_stream() -> stream_inlet 建 TCP 数据连接 -> 循环拉样本。
// 每秒打印一次统计,用于验证链路闭环(应看到 ~2000 样本/s、32 通道、µV 波动)。
//
// 依赖(与本工程共用):
//   - include/lsl_cpp.h  include/lsl_c.h  include/lsl/...
//   - lib/liblsl.so
//
// 编译:
//   g++ -O2 -std=c++17 -I include rt_reader.cpp \
//       -o bin/rt_reader -L lib -llsl -Wl,-rpath,'$ORIGIN/../lib'
//
// 用法:
//   ./bin/rt_reader                    # 拉 type=EEG 的第一个流
//   ./bin/rt_reader --type EEG --name cwz_04151049
//   ./bin/rt_reader --duration 5       # 只拉 5 秒后自动退出
//   Ctrl+C 结束
//
// 说明(将来接 Qt 时的分工):
//   1) 解析入口(本 main) -> Qt main/QApplication
//   2) resolve + stream_inlet + 拉取循环 -> QThread 里的 run()
//   3) 每轮把样本 buf 转 QVector 后用信号发给 GUI 线程
//   4) 本文件里统计/打印的代码 -> GUI 上的数字/波形
// ============================================================================

                                     // TODO
#include "include/lsl_cpp.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using std::chrono::steady_clock;

static std::atomic<bool> g_running{true};

static void onSignal(int) { g_running = false; }

// ---------------------------------------------------------------------------
// 参数
// ---------------------------------------------------------------------------
struct Options {
    std::string type = "EEG";
    std::string name;         // 为空则取该类型第一个流
    double resolveTimeout = 5.0;
    double openTimeout = 5.0;
    double duration = 0.0;    // 0 = 直到 Ctrl+C
    int pullIntervalMs = 200; // 每轮 pull 超时
};

static void printUsage(const char* prog) {
    std::printf(
        "用法: %s [选项]\n"
        "  LSL 拉流端:发现 EEG 流并打印实时统计(配合 lsl_sim 验证链路)\n\n"
        "选项:\n"
        "  --type <s>     要拉的流类型(默认 EEG)\n"
        "  --name <s>     指定流名(默认取该类型第一个发现的流)\n"
        "  --timeout <x>  resolve 探测超时秒(默认 5)\n"
        "  --duration <x> 拉取秒数,默认 0=直到 Ctrl+C\n"
        "  -h, --help     帮助\n",
        prog);
}

static bool parseArgs(int argc, char** argv, Options& o) {
    auto need = [&](int& i, const char* what) {
        if (i + 1 >= argc) {
            std::fprintf(stderr, "[错误] %s 需要参数\n", what);
            std::exit(2);
        }
        return std::string(argv[++i]);
    };
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-h" || a == "--help") { printUsage(argv[0]); std::exit(0); }
        else if (a == "--type") o.type = need(i, a.c_str());
        else if (a == "--name") o.name = need(i, a.c_str());
        else if (a == "--timeout") o.resolveTimeout = std::stod(need(i, a.c_str()));
        else if (a == "--duration") o.duration = std::stod(need(i, a.c_str()));
        else {
            std::fprintf(stderr, "[错误] 未知选项: %s\n", a.c_str());
            printUsage(argv[0]);
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// 主流程
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    Options opt;
    if (!parseArgs(argc, argv, opt)) return 2;

    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    // ---- 1) UDP 探测:找到匹配的流(resolve 阶段) ----
    std::printf("[1] resolve: 寻找 type=%s (超时 %.0fs)...\n",
                opt.type.c_str(), opt.resolveTimeout);
    std::vector<lsl::stream_info> streams;
    try {
        if (opt.name.empty())
            streams = lsl::resolve_stream("type", opt.type, 1, opt.resolveTimeout);
        else
            streams = lsl::resolve_stream("name", opt.name, 1, opt.resolveTimeout);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[错误] resolve 异常: %s\n", e.what());
        return 1;
    }
    if (streams.empty()) {
        std::fprintf(stderr,
                     "[错误] 没发现匹配的流。请确认模拟器正在运行:\n"
                     "       ./bin/lsl_sim --file data/cwz_04151049.bdf --loop\n");
        return 1;
    }
    lsl::stream_info info = streams[0];
    const int nch = info.channel_count();
    const double fs  = info.nominal_srate();
    std::printf("[1] 发现流: %s | type=%s | ch=%d | 标称%.1fHz | source_id=%s\n",
                info.name().c_str(), info.type().c_str(), nch, fs,
                info.source_id().c_str());
    if (nch <= 0) { std::fprintf(stderr, "[错误] 通道数异常\n"); return 1; }

    // ---- 2) 建 inlet 并打开 TCP 数据连接 ----
    std::printf("[2] 打开 inlet(TCP 数据连接)...\n");
    lsl::stream_inlet inlet(info);
    try {
        inlet.open_stream(opt.openTimeout);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[错误] open_stream 失败: %s\n", e.what());
        return 1;
    }
    std::printf("[3] 开始拉取(Ctrl+C 结束)...\n\n");

    // ---- 3) 拉取循环 + 每秒统计 ----
    const size_t kMaxSamp = 4096;   // 每轮最多拉多少"样本"(时间戳缓冲容量)
    // 高性能重载参数语义(易错点):
    //   第3参 data_buffer_elements      按"通道值(元素)"计,须是 nch 的倍数
    //   第4参 timestamp_buffer_elements 按"样本"计,须 = data 能容纳的样本数
    //   第5参 timeout
    //   返回 = 写入 data buffer 的"通道值个数";样本数 = 返回值 / nch
    std::vector<float> buf(static_cast<size_t>(nch) * kMaxSamp);  // 交错: buf[s*nch+c]
    std::vector<double> tss(kMaxSamp);                            // 每样本一个时间戳
    const size_t dataElems = buf.size();   // = nch * kMaxSamp 个 float

    long long totalSamples = 0;      // 累计样本数
    long long winSamples = 0;        // 本 1s 窗口样本数
    auto winStart = steady_clock::now();
    const auto tStart = steady_clock::now();
    std::vector<float> lastSample(nch, 0.f);   // 最近一个样本
    bool haveLast = false;

    std::printf("%8s %12s %8s  %s\n", "秒", "样本/秒", "累计", "最近样本前8通道(µV)");
    while (g_running) {
        std::size_t nElem = 0;
        try {
            nElem = inlet.pull_chunk_multiplexed(
                buf.data(), tss.data(), dataElems, kMaxSamp,
                opt.pullIntervalMs / 1000.0);
        } catch (const std::exception& e) {
            std::fprintf(stderr, "\n[错误] 拉取异常(链路断开?): %s\n", e.what());
            break;
        }
        const std::size_t n = nElem / static_cast<size_t>(nch);  // 实际样本数
        if (n > 0) {
            // 记录本批最后一个样本作为"最近样本"
            const size_t base = (n - 1) * static_cast<size_t>(nch);
            for (int c = 0; c < nch; ++c)
                lastSample[c] = buf[base + static_cast<size_t>(c)];
            haveLast = true;
            totalSamples += static_cast<long long>(n);
            winSamples += static_cast<long long>(n);
        }

        // 每秒统计
        const auto now = steady_clock::now();
        const double winSec = std::chrono::duration<double>(now - winStart).count();
        if (winSec >= 1.0) {
            const double rate = static_cast<double>(winSamples) / winSec;
            const double runSec = std::chrono::duration<double>(now - tStart).count();
            std::printf("%7.1f %11.0f %9lld  ", runSec, rate, totalSamples);
            if (haveLast) {
                for (int c = 0; c < std::min(nch, 8); ++c)
                    std::printf("%8.1f", lastSample[c]);
            }
            std::printf("\n");
            std::fflush(stdout);
            winSamples = 0;
            winStart = now;
        }

        if (opt.duration > 0.0 &&
            std::chrono::duration<double>(now - tStart).count() >= opt.duration)
            break;
    }

    // ---- 4) 汇总 ----
    const double runSec = std::chrono::duration<double>(steady_clock::now() - tStart).count();
    std::printf("\n[完成] 运行 %.1f s, 共拉取 %lld 个样本 x %d 通道\n",
                runSec, totalSamples, nch);
    if (runSec > 0.1)
        std::printf("        实际平均速率: %.0f 样本/s (期望约 %.0f)\n",
                    static_cast<double>(totalSamples) / runSec, fs);
    return 0;
}
