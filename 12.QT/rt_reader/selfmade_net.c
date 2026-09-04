/* ============================================================================
 * selfmade_net.c —— 自研 mini 版 LSL 网络层(学习/求职用, 不依赖 liblsl)
 * ----------------------------------------------------------------------------
 * 用原生 POSIX socket 重写 LSL 那层网络协议, 只做最核心的两个机制:
 *   1) UDP 广播服务发现(等价 LSL 组播 16571):
 *        pull 发 "LSLMINI QUERY" 广播 -> push 收到后单播回 ACK(含 TCP 端口/元数据)
 *   2) TCP 数据流传输(等价 LSL 16572~16604):
 *        push 作为 server, 每 ~100ms 按 2000Hz 把 32ch float 帧发给所有已连 client
 *        pull 作为 client, 用自定义帧头做"粘包/拆包"精确接收
 *
 * 首版数据用内置正弦+噪声模拟 32 通道 @2000Hz(先专注网络, 不背 BDF 包袱)。
 *
 * 编译:  gcc -O2 -Wall -pthread selfmade_net.c -o bin/selfmade_net -lm
 *
 * 用法:
 *   终端A(push/发射): ./bin/selfmade_net --role push --ch 32 --fs 2000
 *   终端B(pull/接收): ./bin/selfmade_net --role pull --ip 127.0.0.1 --duration 5
 *   跨机器: pull 的 --ip 填 push 所在机器 IP(广播默认发到 255.255.255.255)
 *
 * 这里练到的网络工程点:
 *   - socket/bind/listen/accept/connect/sendto/recvfrom
 *   - UDP 广播: SO_BROADCAST, 目的 255.255.255.255
 *   - TCP 粘包/拆包: 定长帧头(magic+length) + 精确 recv 循环
 *   - 字节序: 手动 put16/put32/put64 打包网络序(端到端教学)
 *   - select 同时监听 UDP + TCP; 多客户端(accept 数组); 多线程(pthread)
 *   - 阻塞/超时: SO_RCVTIMEO、select 超时、优雅退出(Ctrl+C)
 * ========================================================================== */

                                            /* TODO */

#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <math.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

/* ---------------- 协议常量 ---------------- */
#define PROTO_QUERY  "LSLMINI QUERY"   /* UDP 查询(广播) */
#define PROTO_INFO   "LSLMINI INFO "    /* UDP 应答前缀 */
#define MAGIC        0x4C534C31UL      /* "LSL1" */
#define HDR_BYTES    28                /* 数据帧头字节数 */
#define DEFAULT_UDP  16571             /* 发现端口 */
#define MAX_CLIENTS  8
#define CHUNK        200               /* 每帧样本数(100ms @2000Hz) */

/* 数据帧头(28B, 全部按网络序在 wire 上传输): */
/*   magic(4) len(4) nch(2) nsamp(2) seq(8) t0_us(8)                       */
/*   payload = nsamp*nch 个 float32(小端, 交错排列 sample*nch+ch)          */
typedef struct {
    uint32_t magic;
    uint32_t payload_len;
    uint16_t nch;
    uint16_t nsamp;
    uint64_t seq;    /* 本帧第一个样本的全局序号 */
    int64_t  t0_us;  /* 本帧第一个样本的单调时钟(us) */
} FrameHeader;

/* ---------------- 全局状态 ---------------- */
static volatile sig_atomic_t g_running = 1;
static void on_sig(int s) { (void)s; g_running = 0; }

/* ---------------- 工具: 网络字节序打包/解包 ---------------- */
static void put16(uint8_t *b, size_t *p, uint16_t v) {
    b[(*p)++] = (uint8_t)(v >> 8);  b[(*p)++] = (uint8_t)(v & 0xff);
}
static void put32(uint8_t *b, size_t *p, uint32_t v) {
    b[(*p)++] = (uint8_t)(v >> 24); b[(*p)++] = (uint8_t)(v >> 16);
    b[(*p)++] = (uint8_t)(v >> 8);  b[(*p)++] = (uint8_t)(v);
}
static void put64(uint8_t *b, size_t *p, uint64_t v) {
    for (int i = 7; i >= 0; --i) b[(*p)++] = (uint8_t)(v >> (8 * i));
}
static uint16_t get16(const uint8_t *b, size_t *p) {
    uint16_t v = ((uint16_t)b[*p] << 8) | b[*p + 1]; *p += 2; return v;
}
static uint32_t get32(const uint8_t *b, size_t *p) {
    uint32_t v = 0; for (int i = 0; i < 4; ++i) v = (v << 8) | b[(*p)++]; return v;
}
static uint64_t get64(const uint8_t *b, size_t *p) {
    uint64_t v = 0; for (int i = 0; i < 8; ++i) v = (v << 8) | b[(*p)++]; return v;
}

static void pack_header(uint8_t wire[HDR_BYTES], const FrameHeader *h) {
    size_t p = 0;
    put32(wire, &p, h->magic);
    put32(wire, &p, h->payload_len);
    put16(wire, &p, h->nch);
    put16(wire, &p, h->nsamp);
    put64(wire, &p, h->seq);
    put64(wire, &p, (uint64_t)h->t0_us);
}
static void unpack_header(const uint8_t wire[HDR_BYTES], FrameHeader *h) {
    size_t p = 0;
    h->magic = get32(wire, &p);
    h->payload_len = get32(wire, &p);
    h->nch = get16(wire, &p);
    h->nsamp = get16(wire, &p);
    h->seq = get64(wire, &p);
    h->t0_us = (int64_t)get64(wire, &p);
}

/* 单调时钟 -> 微秒 */
static int64_t mono_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

/* 精确收满 n 字节(处理 TCP 拆分) */
static int recv_full(int fd, void *buf, size_t n) {
    uint8_t *p = buf;
    size_t got = 0;
    while (got < n && g_running) {
        ssize_t r = recv(fd, p + got, n - got, 0);
        if (r <= 0) return -1;          /* 断开/出错 */
        got += (size_t)r;
    }
    return g_running ? 0 : -1;
}
/* 精确发完 n 字节(处理部分发送/对端断开) */
static int send_full(int fd, const void *buf, size_t n) {
    const uint8_t *p = buf;
    size_t sent = 0;
    while (sent < n) {
        ssize_t r = send(fd, p + sent, n - sent, MSG_NOSIGNAL);
        if (r <= 0) return -1;          /* EPIPE/ECONNRESET 等 */
        sent += (size_t)r;
    }
    return 0;
}

/* ============================================================================
 * 角色 A —— push(发射端/server): UDP 应答发现 + TCP 广播数据
 * ========================================================================== */
struct PushCfg {
    int udp_port, nch;
    double fs;
};
static int g_clients[MAX_CLIENTS];
static pthread_mutex_t g_cli_lock = PTHREAD_MUTEX_INITIALIZER;

static void add_client(int fd) {
    pthread_mutex_lock(&g_cli_lock);
    for (int i = 0; i < MAX_CLIENTS; ++i)
        if (g_clients[i] < 0) { g_clients[i] = fd; break; }
    pthread_mutex_unlock(&g_cli_lock);
}

/* 数据发送线程: 生成正弦 -> 组帧 -> 发给所有已连接 client */
static void *push_sender(void *arg) {
    struct PushCfg *cfg = arg;
    const int nch = cfg->nch;
    const int chunk = CHUNK;
    const size_t payload_bytes = (size_t)chunk * nch * sizeof(float);

    uint8_t *frame = malloc(HDR_BYTES + payload_bytes);
    if (!frame) return NULL;
    float *samples = (float *)(frame + HDR_BYTES);

    /* 每通道独立频率/幅度, 让波形可区分 */
    double freq[64], amp[64], ph[64];
    for (int c = 0; c < nch && c < 64; ++c) {
        freq[c] = 1.0 + (c % 10) * 2.0;      /* 1~19 Hz 间隔 */
        amp[c] = 40.0 + (c * 37) % 90;        /* uV 量级 */
        ph[c] = 0.0;
    }
    srand(42);

    uint64_t s0 = 0;  /* 全局样本序号 */
    int64_t tstart = mono_us();
    int64_t next_us = tstart;   /* 节奏: 单调推进 */
    FrameHeader h;
    h.magic = MAGIC; h.nch = (uint16_t)nch; h.nsamp = (uint16_t)chunk;
    h.payload_len = (uint32_t)payload_bytes;

    while (g_running) {
        /* ---- 1) 生成 200 样本(交错存 sample*nch+ch) ---- */
        for (int k = 0; k < chunk; ++k) {
            double t = (double)(s0 + k) / cfg->fs;
            for (int c = 0; c < nch; ++c) {
                double v = amp[c] * sin(2 * M_PI * freq[c] * t + ph[c]);
                v += (double)(rand() % 2000 - 1000) / 100.0;  /* 小噪声 */
                samples[(size_t)k * nch + c] = (float)v;
            }
        }
        h.seq = s0;
        h.t0_us = next_us;
        pack_header(frame, &h);

        /* ---- 2) 发给每个客户端(TCP) ---- */
        pthread_mutex_lock(&g_cli_lock);
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (g_clients[i] >= 0) {
                if (send_full(g_clients[i], frame, HDR_BYTES + payload_bytes) != 0) {
                    close(g_clients[i]);
                    g_clients[i] = -1;   /* 对端断开即移除 */
                }
            }
        }
        pthread_mutex_unlock(&g_cli_lock);

        s0 += (uint64_t)chunk;
        /* ---- 3) 节奏控制: 每帧 chunk/fs 秒, 单调累加目标时刻 ---- */
        next_us += (int64_t)((double)chunk / cfg->fs * 1e6);
        struct timespec ts = { next_us / 1000000, (next_us % 1000000) * 1000 };
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL);
    }
    free(frame);
    return NULL;
}

static int run_push(const struct PushCfg *cfg) {
    for (int i = 0; i < MAX_CLIENTS; ++i) g_clients[i] = -1;

    /* ---- UDP 发现 socket ---- */
    int udp = socket(AF_INET, SOCK_DGRAM, 0);
    int on = 1;
    setsockopt(udp, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
    struct sockaddr_in ua;
    memset(&ua, 0, sizeof(ua));
    ua.sin_family = AF_INET;
    ua.sin_addr.s_addr = htonl(INADDR_ANY);
    ua.sin_port = htons((uint16_t)cfg->udp_port);
    if (bind(udp, (struct sockaddr *)&ua, sizeof(ua)) != 0) {
        perror("bind udp"); return 1;
    }

    /* ---- TCP 监听 socket(端口 0 = 系统自动分配) ---- */
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    struct sockaddr_in ta;
    memset(&ta, 0, sizeof(ta));
    ta.sin_family = AF_INET;
    ta.sin_addr.s_addr = htonl(INADDR_ANY);
    ta.sin_port = htons(0);
    if (bind(lfd, (struct sockaddr *)&ta, sizeof(ta)) != 0) { perror("bind tcp"); return 1; }
    listen(lfd, 8);
    socklen_t tl = sizeof(ta);
    getsockname(lfd, (struct sockaddr *)&ta, &tl);
    int tcp_port = ntohs(ta.sin_port);

    printf("[push] UDP:%d(发现)  TCP:%d(数据)  ch=%d fs=%.0fHz  (Ctrl+C 退出)\n",
           cfg->udp_port, tcp_port, cfg->nch, cfg->fs);

    pthread_t th;
    pthread_create(&th, NULL, push_sender, (void *)cfg);

    /* ---- 主循环: select 同时处理 UDP 查询 和 TCP accept ---- */
    while (g_running) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(udp, &rfds);
        FD_SET(lfd, &rfds);
        struct timeval tv = { 0, 500000 };   /* 0.5s 超时, 便于检查退出 */
        if (select(lfd + 1, &rfds, NULL, NULL, &tv) < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (FD_ISSET(udp, &rfds)) {
            char buf[256];
            struct sockaddr_in from;
            socklen_t fl = sizeof(from);
            ssize_t n = recvfrom(udp, buf, sizeof(buf) - 1, 0,
                                 (struct sockaddr *)&from, &fl);
            if (n > 0) {
                buf[n] = 0;
                if (strncmp(buf, PROTO_QUERY, strlen(PROTO_QUERY)) == 0) {
                    char r[256];
                    int rn = snprintf(r, sizeof(r),
                        "%sname=EEGSIM type=EEG ch=%d fs=%.0f tcp=%d\n",
                        PROTO_INFO, cfg->nch, cfg->fs, tcp_port);
                    sendto(udp, r, (size_t)rn, 0,
                           (struct sockaddr *)&from, sizeof(from));
                    printf("[push] 应答发现查询 (来自 %s)\n",
                           inet_ntoa(from.sin_addr));
                }
            }
        }
        if (FD_ISSET(lfd, &rfds)) {
            struct sockaddr_in cli;
            socklen_t cl = sizeof(cli);
            int cfd = accept(lfd, (struct sockaddr *)&cli, &cl);
            if (cfd >= 0) {
                int nd = 1;
                setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, &nd, sizeof(nd));
                add_client(cfd);
                printf("[push] 新客户端连接: %s:%d\n",
                       inet_ntoa(cli.sin_addr), ntohs(cli.sin_port));
            }
        }
    }

    printf("\n[push] 退出, 关闭所有连接...\n");
    close(udp);
    close(lfd);
    pthread_mutex_lock(&g_cli_lock);
    for (int i = 0; i < MAX_CLIENTS; ++i)
        if (g_clients[i] >= 0) close(g_clients[i]);
    pthread_mutex_unlock(&g_cli_lock);
    return 0;
}

/* ============================================================================
 * 角色 B —— pull(接收端/client): UDP 广播查询 -> TCP 连接 -> 拆帧拉取
 * ========================================================================== */
static int run_pull(const char *ip, int udp_port, double duration) {
    /* ---- 1) UDP 广播查询 ---- */
    int udp = socket(AF_INET, SOCK_DGRAM, 0);
    int on = 1;
    setsockopt(udp, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
    struct timeval to = { 4, 0 };
    setsockopt(udp, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to));

    struct sockaddr_in dst;
    memset(&dst, 0, sizeof(dst));
    dst.sin_family = AF_INET;
    dst.sin_port = htons((uint16_t)udp_port);
    inet_pton(AF_INET, ip, &dst.sin_addr);

    printf("[pull] 广播查询 type=EEG ...\n");
    sendto(udp, PROTO_QUERY, strlen(PROTO_QUERY), 0,
           (struct sockaddr *)&dst, sizeof(dst));

    int tcp_port = -1, nch = -1;
    double fs = 0.0;
    char buf[256];
    struct sockaddr_in from;
    socklen_t fl = sizeof(from);
    ssize_t n = recvfrom(udp, buf, sizeof(buf) - 1, 0,
                         (struct sockaddr *)&from, &fl);
    if (n > 0) {
        buf[n] = 0;
        printf("[pull] 收到应答: %s", buf);
        if (sscanf(buf, PROTO_INFO "name=%*s type=%*s ch=%d fs=%lf tcp=%d",
                   &nch, &fs, &tcp_port) < 3)
            tcp_port = -1;
    }
    close(udp);
    if (tcp_port <= 0 || nch <= 0 || fs <= 0.0) {
        fprintf(stderr, "[pull] 未发现可用的 push(检查 --ip/--udp 是否可达)\n");
        return 1;
    }

    /* ---- 2) TCP 连接 ---- */
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons((uint16_t)tcp_port);
    inet_pton(AF_INET, ip, &sa.sin_addr);
    if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) != 0) {
        perror("connect"); return 1;
    }
    printf("[pull] 已连接 TCP:%d  ch=%d fs=%.0fHz\n", tcp_port, nch, fs);

    /* ---- 3) 拆帧拉取(粘包处理的核心) ---- */
    struct timeval rt = { 0, 500000 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &rt, sizeof(rt));

    float *last = malloc((size_t)nch * sizeof(float));
    long long totalSamples = 0, winSamples = 0;
    double tStart = (double)mono_us() / 1e6, winStart = tStart;

    printf("%8s %12s %9s  %s\n", "秒", "样本/秒", "累计", "最近前4通道(µV)");
    while (g_running) {
        uint8_t wire[HDR_BYTES];
        if (recv_full(fd, wire, HDR_BYTES) != 0) break;      /* 先收满帧头 */
        FrameHeader h;
        unpack_header(wire, &h);
        if (h.magic != MAGIC || h.payload_len != (uint32_t)h.nch * h.nsamp * 4) {
            fprintf(stderr, "[pull] 帧头非法(magic/长度校验失败) — 说明粘包拆错!\n");
            break;
        }
        float *samples = malloc(h.payload_len);
        if (!samples) break;
        if (recv_full(fd, samples, h.payload_len) != 0) { free(samples); break; }
        memcpy(last, samples + ((size_t)h.nsamp - 1) * h.nch,
               (size_t)h.nch * sizeof(float));

        totalSamples += (long long)h.nsamp;
        winSamples += (long long)h.nsamp;
        free(samples);

        double now = (double)mono_us() / 1e6;
        if (now - winStart >= 1.0) {
            printf("%7.1f %11.0f %9lld  ", now - tStart,
                   (double)winSamples / (now - winStart), totalSamples);
            for (int c = 0; c < nch && c < 4; ++c)
                printf("%8.1f", last[c]);
            printf("\n");
            winSamples = 0;
            winStart = now;
        }
        if (duration > 0.0 && now - tStart >= duration) break;
    }
    free(last);
    close(fd);
    double runSec = (double)mono_us() / 1e6 - tStart;
    printf("\n[pull] 运行 %.1fs, 共 %lld 样本 x %d 通道 (平均 %.0f/s)\n",
           runSec, totalSamples, nch,
           runSec > 0 ? (double)totalSamples / runSec : 0.0);
    return 0;
}

/* ---------------- 入口 ---------------- */
static void usage(const char *prog) {
    fprintf(stderr,
        "用法: %s --role push|pull [选项]\n"
        "  push(发射端/server):\n"
        "    [--udp <port>] 发现端口, 默认 %d\n"
        "    [--ch <n>] 通道数, 默认 32\n"
        "    [--fs <hz>] 采样率, 默认 2000\n"
        "  pull(接收端/client):\n"
        "    --ip <addr>     push 的地址(同机 127.0.0.1; 广播默认发 255.255.255.255 需 --ip)\n"
        "    [--udp <port>] 发现端口, 默认 %d\n"
        "    [--duration <s>] 拉取秒数, 默认 0=直到 Ctrl+C\n",
        prog, DEFAULT_UDP, DEFAULT_UDP);
}
int main(int argc, char **argv) {
    const char *role = NULL, *ip = "255.255.255.255";
    int udp_port = DEFAULT_UDP, nch = 32;
    double fs = 2000.0, duration = 0.0;

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--role") && i + 1 < argc) role = argv[++i];
        else if (!strcmp(argv[i], "--ip") && i + 1 < argc) ip = argv[++i];
        else if (!strcmp(argv[i], "--udp") && i + 1 < argc) udp_port = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--ch") && i + 1 < argc) nch = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--fs") && i + 1 < argc) fs = atof(argv[++i]);
        else if (!strcmp(argv[i], "--duration") && i + 1 < argc) duration = atof(argv[++i]);
        else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) { usage(argv[0]); return 0; }
        else { fprintf(stderr, "未知参数: %s\n", argv[i]); usage(argv[0]); return 2; }
    }
    if (!role) { usage(argv[0]); return 2; }

    signal(SIGINT, on_sig);
    signal(SIGTERM, on_sig);

    if (!strcmp(role, "push")) {
        struct PushCfg cfg = { udp_port, nch, fs };
        return run_push(&cfg);
    } else if (!strcmp(role, "pull")) {
        return run_pull(ip, udp_port, duration);
    }
    fprintf(stderr, "--role 只能为 push 或 pull\n");
    return 2;
}
