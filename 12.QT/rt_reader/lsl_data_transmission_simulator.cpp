// ============================================================================
// lsl_data_transmission_simulator.cpp
// ----------------------------------------------------------------------------
// 用 data/*.bdf (BioSemi 24-bit BDF+C) 模拟一台"发射设备"：
// 解析 BDF 头 -> 解码 24-bit EEG -> 通过 LSL(liblsl C++) 在局域网上推流。
// LSL 底层即"UDP 广播(服务发现,组播 16571) + TCP(数据传输,16572~16604)"，
// 这些网络细节由 liblsl 封装，这里只需创建 stream_outlet 再灌数据即可。
//
// 依赖(不在本仓库、需要自行准备)：
//   - lib/lsl_cpp.h        官方 C++ 头文件(sccn/liblsl)
//   - liblsl.so            官方共享库(liblsl)
//
// 编译示例：
//   g++ -O2 -std=c++17 lsl_data_transmission_simulator.cpp \
//       -o bin/lsl_sim -I lib -llsl
//   # liblsl.so 不在默认路径时：
//   g++ -O2 -std=c++17 lsl_data_transmission_simulator.cpp \
//       -o bin/lsl_sim -I lib -L/path/to/liblsl -Wl,-rpath,/path/to/liblsl -llsl
//
// 用法示例：
//   ./bin/lsl_sim --file data/cwz_04151049.bdf          # 只播一遍
//   ./bin/lsl_sim --file data/zty_04251731.bdf --loop   # 循环回放
//   ./bin/lsl_sim --file data/cwz_04151049.bdf --rate 0.5   # 0.5 倍速
//   ./bin/lsl_sim --file data/cwz_04151049.bdf --wait   # 等接收端连上再开始
// ============================================================================

#include "include/lsl_cpp.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using std::chrono::duration;
using std::chrono::steady_clock;

// ---------------------------------------------------------------------------
// 小工具
// ---------------------------------------------------------------------------
static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static std::string asciiField(const uint8_t* p, size_t len) {
    return trim(std::string(reinterpret_cast<const char*>(p), len));
}

static double asciiNum(const uint8_t* p, size_t len, double def = 0.0) {
    std::string s = asciiField(p, len);
    if (s.empty()) return def;
    try {
        return std::stod(s);
    } catch (...) {
        return def;
    }
}

// ---------------------------------------------------------------------------
// BDF / EDF 头解析(主头 256B + "列式"通道头)
// 布局: labels(ns*16) transducer(ns*80) dim(ns*8) pmin/pmax/dmin/dmax(ns*8)
//       prefiltering(ns*80) nsamp(ns*8) reserved(ns*32)
// ---------------------------------------------------------------------------
struct Channel {
    std::string label;   // 如 FP2
    std::string dim;     // 如 uV
    double pmin = 0, pmax = 0;
    int dmin = 0, dmax = 0;
    int sps = 0;         // 每个数据记录里的样本数(通常即采样率)
};

struct BdfInfo {
    std::string path;
    std::string startdate, starttime;
    long hdrBytes = 0;    // 头总长,数据从这里开始
    long nRecords = -1;   // 数据记录数(-1 未知)
    double recDur = 1.0;  // 每条记录时长(秒)
    int ns = 0;           // 通道总数(含注解通道)
    std::vector<Channel> channels;
    std::vector<int> eeg; // 要推流的数据通道下标
    double fs = 0.0;      // 采样率(Hz)
    size_t recordBytes = 0;  // 一条记录的字节数
};

static bool isAnnotationChannel(const Channel& c) {
    std::string l = c.label;
    std::transform(l.begin(), l.end(), l.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return l.find("annotation") != std::string::npos || c.dim.empty();
}

static bool parseBdfHeader(const std::string& path, BdfInfo& b) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        std::cerr << "[错误] 无法打开文件: " << path << "\n";
        return false;
    }
    uint8_t hdr[256];
    f.read(reinterpret_cast<char*>(hdr), 256);
    if (f.gcount() < 256) {
        std::cerr << "[错误] 文件过短,不是有效的 BDF/EDF 文件\n";
        return false;
    }
    auto txt = [&](int a, int len) { return asciiField(hdr + a, len); };
    auto num = [&](int a, int len, double def) { return asciiNum(hdr + a, len, def); };

    int ns = static_cast<int>(num(252, 4, 0));  // 通道数
    if (ns <= 0 || ns > 512) {
        std::cerr << "[错误] 通道数异常: " << ns << "\n";
        return false;
    }
    b.path = path;
    b.startdate = txt(168, 8);
    b.starttime = txt(176, 8);
    b.hdrBytes = static_cast<long>(num(184, 8, 0));
    b.nRecords = static_cast<long>(num(236, 8, -1));
    b.recDur = num(244, 8, 1.0);
    b.ns = ns;
    if (b.hdrBytes <= 0) b.hdrBytes = 256 + static_cast<long>(ns) * 256;

    // 读取各"字段列"
    std::vector<uint8_t> lb, tr, dm, pn, px, dn, dx, pf, np;
    auto readCol = [&](size_t width, std::vector<uint8_t>& out) {
        out.resize(width * static_cast<size_t>(ns));
        f.read(reinterpret_cast<char*>(out.data()),
               static_cast<std::streamsize>(out.size()));
    };
    readCol(16, lb); readCol(80, tr); readCol(8, dm);
    readCol(8, pn); readCol(8, px); readCol(8, dn);
    readCol(8, dx); readCol(80, pf); readCol(8, np);
    // 剩余 reserved(ns*32) 不再需要

    auto col = [&](const std::vector<uint8_t>& v, int i, size_t w) {
        return v.data() + static_cast<size_t>(i) * w;
    };
    b.channels.resize(ns);
    for (int i = 0; i < ns; ++i) {
        Channel& c = b.channels[i];
        c.label = asciiField(col(lb, i, 16), 16);
        c.dim = asciiField(col(dm, i, 8), 8);
        c.pmin = asciiNum(col(pn, i, 8), 8, 0.0);
        c.pmax = asciiNum(col(px, i, 8), 8, 0.0);
        c.dmin = static_cast<int>(asciiNum(col(dn, i, 8), 8, 0.0));
        c.dmax = static_cast<int>(asciiNum(col(dx, i, 8), 8, 0.0));
        c.sps = static_cast<int>(asciiNum(col(np, i, 8), 8, 0.0));
        if (!isAnnotationChannel(c)) b.eeg.push_back(i);
    }
    if (b.eeg.empty()) {
        std::cerr << "[错误] 没有可推流的数据通道(可能全部被识别为注解通道)\n";
        return false;
    }
    b.fs = b.channels[b.eeg[0]].sps / b.recDur;

    long totalSamp = 0;
    for (const auto& c : b.channels) totalSamp += c.sps;
    b.recordBytes = static_cast<size_t>(totalSamp) * 3;  // BDF: 每样本 3 字节
    return true;
}

// ---------------------------------------------------------------------------
// 数据解码
// ---------------------------------------------------------------------------
// 3 字节小端有符号 24-bit -> int32
static inline int32_t decode24(const uint8_t* p) {
    int32_t v = static_cast<int32_t>(p[0]) | (static_cast<int32_t>(p[1]) << 8)
                | (static_cast<int32_t>(p[2]) << 16);
    if (v & 0x800000) v |= ~0xFFFFFF;  // 符号扩展
    return v;
}

// 数字原始值 -> 物理值(µV),用每通道的标定范围换算
static inline double applyCal(const Channel& c, int32_t raw) {
    double dig = static_cast<double>(c.dmax) - c.dmin;
    double phy = c.pmax - c.pmin;
    if (dig == 0.0) return static_cast<double>(raw);
    return (static_cast<double>(raw) - c.dmin) / dig * phy + c.pmin;
}

// ---------------------------------------------------------------------------
// 主流程
// ---------------------------------------------------------------------------
struct Options {
    std::string file = "data/cwz_04151049.bdf";
    std::string name;     // 为空时自动取文件名
    std::string type = "EEG";
    std::string sourceId;
    double rate = 1.0;    // 回放倍速
    int chunk = 200;      // 每块样本数
    bool loop = false;
    bool wait = false;
    bool verbose = true;
};

static void printUsage(const char* prog) {
    std::cout
        << "用法: " << prog << " [选项]\n"
        << "  用 .bdf 文件模拟一台 32 通道 EEG 设备在局域网上 LSL 推流\n\n"
        << "选项:\n"
        << "  --file <path>     BDF 文件(默认 data/cwz_04151049.bdf)\n"
        << "  --name <s>        流名(默认取文件名,如 cwz_04151049)\n"
        << "  --type <s>        流类型(默认 EEG)\n"
        << "  --sourceid <s>    源 ID(默认同流名)\n"
        << "  --rate <x>        回放倍速,默认 1.0(>1 快放,<1 慢放)\n"
        << "  --chunk <n>       每块样本数(推送粒度),默认 200\n"
        << "  --loop            文件播完后循环回放\n"
        << "  --wait            等待至少一个接收端(Inlet)连上再开始\n"
        << "  -h, --help        显示本帮助\n";
}

static bool parseArgs(int argc, char** argv, Options& o) {
    auto need = [&](int& i, const char* what) -> std::string {
        if (i + 1 >= argc) {
            std::cerr << "[错误] 选项 " << what << " 需要一个参数\n";
            std::exit(2);
        }
        return argv[++i];
    };
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-h" || a == "--help") {
            printUsage(argv[0]);
            std::exit(0);
        } else if (a == "--file") o.file = need(i, a.c_str());
        else if (a == "--name") o.name = need(i, a.c_str());
        else if (a == "--type") o.type = need(i, a.c_str());
        else if (a == "--sourceid") o.sourceId = need(i, a.c_str());
        else if (a == "--rate") o.rate = std::stod(need(i, a.c_str()));
        else if (a == "--chunk") o.chunk = std::stoi(need(i, a.c_str()));
        else if (a == "--loop") o.loop = true;
        else if (a == "--wait") o.wait = true;
        else {
            std::cerr << "[错误] 未知选项: " << a << "\n";
            printUsage(argv[0]);
            return false;
        }
    }
    if (o.rate <= 0.0) o.rate = 1.0;
    if (o.chunk <= 0) o.chunk = 200;
    return true;
}

static std::string baseName(const std::string& path) {
    std::string p = path;
    size_t slash = p.find_last_of("/\\");
    if (slash != std::string::npos) p = p.substr(slash + 1);
    size_t dot = p.find_last_of('.');
    if (dot != std::string::npos) p = p.substr(0, dot);
    return p;
}

int main(int argc, char** argv) {
    Options opt;
    if (!parseArgs(argc, argv, opt)) return 2;

    // ---- 1. 解析 BDF 头 ----
    BdfInfo b;
    if (!parseBdfHeader(opt.file, b)) return 1;
    const int nch = static_cast<int>(b.eeg.size());
    const int sps = b.channels[b.eeg[0]].sps;  // 每条记录、每个 EEG 通道的样本数

    std::string streamName = opt.name.empty() ? baseName(opt.file) : opt.name;
    std::string sourceId = opt.sourceId.empty() ? streamName : opt.sourceId;

    std::cout << "== BDF 信息 ==" << "\n"
              << "  文件        : " << b.path << "\n"
              << "  记录时间    : " << b.startdate << " " << b.starttime << "\n"
              << "  总通道数    : " << b.ns << " (数据 " << nch
              << " + 注解 " << (b.ns - nch) << ")\n"
              << "  采样率      : " << b.fs << " Hz\n"
              << "  记录数/时长 : " << b.nRecords << " x " << b.recDur
              << " s ≈ " << (b.nRecords > 0 ? b.nRecords * b.recDur : 0.0)
              << " s\n"
              << "  推流通道    : ";
    for (int c = 0; c < nch; ++c) {
        std::cout << b.channels[b.eeg[c]].label
                  << (c + 1 < nch ? ", " : "");
    }
    std::cout << "\n== LSL Outlet ==" << "\n"
              << "  流名/类型   : " << streamName << " / " << opt.type << "\n"
              << "  源 ID       : " << sourceId << "\n"
              << "  通道数      : " << nch << "\n"
              << "  采样率      : " << b.fs << " Hz (float32)\n"
              << "  倍速/块     : " << opt.rate << "x / " << opt.chunk
              << " 样本每块\n"
              << (opt.loop ? "  循环        : 是\n" : "  循环        : 否\n")
              << (opt.wait ? "  等待接收端  : 是\n" : "  等待接收端  : 否\n");

    // ---- 2. 创建 outlet(创建后即可通过 UDP 被发现)----
    lsl::stream_info info(streamName, opt.type, nch, b.fs,
                          lsl::cf_float32, sourceId);

    // 把通道名写入 XML 元数据,便于接收端识别(LabRecorder 也会记录)
    try {
        auto ch = info.desc().append_child("channels");
        for (int c = 0; c < nch; ++c)
            ch.append_child("channel").append_child_value(
                "label", b.channels[b.eeg[c]].label);
    } catch (std::exception&) { /* 元数据可选,失败忽略 */ }

    lsl::stream_outlet outlet(info, opt.chunk, 360);
    std::cout << "[LSL] Outlet 已创建,等待/推流中…\n";

    if (opt.wait) {
        if (!outlet.wait_for_consumers(20.0))
            std::cout << "[LSL] 20s 内无接收端,仍开始推流(可之后连上)\n";
        else
            std::cout << "[LSL] 检测到接收端,开始推流\n";
    }

    // ---- 3. 打开数据文件,开始流式回放 ----
    std::ifstream f(b.path, std::ios::binary);
    if (!f) { std::cerr << "[错误] 无法打开数据文件\n"; return 1; }
    f.seekg(b.hdrBytes, std::ios::beg);

    // 每条记录中,每个通道的字节偏移(record 内通道按序连续存放)
    std::vector<size_t> chanOff(b.ns + 1, 0);
    for (int i = 0; i < b.ns; ++i)
        chanOff[i + 1] = chanOff[i] + static_cast<size_t>(b.channels[i].sps) * 3;

    std::vector<uint8_t> rec(b.recordBytes);
    // 解码后的本记录 EEG 数据,chan-major: [c * sps + s]
    std::vector<float> chan(nch * static_cast<size_t>(sps));
    // 待推的"交错(multiplexed)"块缓冲:[块内样本 * nch + 通道]
    std::vector<float> xbuf(static_cast<size_t>(opt.chunk) * nch);

    // LSL 时间轴起点(留 0.2s 余量让订阅方建立连接)
    const double t0 = lsl::local_clock() + 0.2;
    long long sampleIdx = 0;   // 已进入推流管道的全局样本计数(单通道)
    size_t fill = 0;           // 当前块已填样本数
    auto wake = steady_clock::now();
    long long lastReport = -5;

    auto emitChunk = [&]() {
        if (fill == 0) return;
        const long long endIdx = sampleIdx + static_cast<long long>(fill) - 1;
        const double ts = t0 + static_cast<double>(endIdx) / b.fs;  // 块末样本时刻
        // 实时节奏控制(rate 倍速)
        // time_point::operator+= 只接受 steady_clock 自身的 duration,
        // 因此把浮点秒 duration<double> 先转成 steady_clock::duration
        wake += std::chrono::duration_cast<steady_clock::duration>(
            duration<double>(static_cast<double>(fill) / (b.fs * opt.rate)));
        std::this_thread::sleep_until(wake);
        outlet.push_chunk_multiplexed(xbuf.data(),
                                      fill * static_cast<size_t>(nch),
                                      ts, true);
        sampleIdx += static_cast<long long>(fill);
        fill = 0;
    };

    bool readRecord = true;
    while (true) {
        f.read(reinterpret_cast<char*>(rec.data()),
               static_cast<std::streamsize>(rec.size()));
        if (f.gcount() < static_cast<std::streamsize>(rec.size())) {
            // 文件读完
            if (!opt.loop) break;
            f.clear();
            f.seekg(b.hdrBytes, std::ios::beg);
            f.read(reinterpret_cast<char*>(rec.data()),
                   static_cast<std::streamsize>(rec.size()));
            if (f.gcount() < static_cast<std::streamsize>(rec.size())) break;
            std::cout << "[LSL] 已到文件末尾,循环回放...\n";
        }
        // 解码本记录(每个 EEG 通道 sps 个样本)
        for (int c = 0; c < nch; ++c) {
            const int chIdx = b.eeg[c];
            const uint8_t* base = rec.data() + chanOff[chIdx];
            for (int s = 0; s < sps; ++s)
                chan[static_cast<size_t>(c) * sps + s] =
                    static_cast<float>(applyCal(b.channels[chIdx],
                                                decode24(base + s * 3)));
        }
        // 交错填充块并推送
        for (int s = 0; s < sps; ++s) {
            for (int c = 0; c < nch; ++c)
                xbuf[fill * static_cast<size_t>(nch) + c] =
                    chan[static_cast<size_t>(c) * sps + s];
            ++fill;
            if (fill == static_cast<size_t>(opt.chunk)) emitChunk();
        }
        // 进度汇报
        const long long secs = sampleIdx / static_cast<long long>(b.fs);
        if (secs / 5 > lastReport / 5) {
            lastReport = secs;
            std::cout << "  已推流 " << secs << " s, 当前时间戳 "
                      << (t0 + static_cast<double>(sampleIdx) / b.fs - lsl::local_clock())
                      << " s\n";
        }
    }
    emitChunk();  // 推掉尾部不足一块的样本

    std::cout << "\n[完成] 共推流 " << sampleIdx << " 个样本 x " << nch
              << " 通道(≈ "
              << (b.fs > 0 ? static_cast<double>(sampleIdx) / b.fs : 0.0)
              << " s)。\n";
    return 0;
}
