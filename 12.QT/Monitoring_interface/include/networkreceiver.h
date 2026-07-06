// networkreceiver.h
#ifndef NETWORKRECEIVER_H
#define NETWORKRECEIVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QVector>
#include <QString>
#include <QElapsedTimer>

// 网络数据接收器 —— TCP 服务端，接收 32 通道放大器(CC3235) 实时脑电数据
// 协议: 设备作为 TCP 客户端连接本机，发送 108 字节定长数据包
//   帧头: 0xA1 + 0x05 (2字节)
//   电池电量: 1字节
//   脱落检测: 5字节
//   包序号: 4字节 (uint32 little-endian)
//   32通道数据: 96字节 (每通道3字节 signed 24-bit)
// PC 端可通过 UDP 向设备发送控制指令:
//   开始传输: 0xBB + 0x66 + 0x01
//   脱落检测: 0xBB + 0x65 + 0x00(开)/0x01(关)
class NetworkReceiver : public QObject
{
    Q_OBJECT

public:
    // 连接参数
    struct Config {
        quint16 listenPort = 5001;      // TCP 监听端口
        int channelCount = 32;           // 固定 32 通道
        int sampleRate = 2000;           // 固定 2kHz 采样率
        QStringList channelLabels;       // 通道标签(默认 32 导标准蒙太奇)
    };

    enum State {
        Disconnected,
        Listening,
        Connected,
        Error
    };
    Q_ENUM(State)

    explicit NetworkReceiver(QObject *parent = nullptr);
    ~NetworkReceiver();

    // === TCP 服务端 ===
    // 开始监听端口，等待设备连接
    void startServer(quint16 port);
    // 停止监听并断开连接
    void stopServer();

    // === UDP 控制指令(PC→设备) ===
    // 发送开始传输指令
    void sendStartCommand(const QString &deviceHost, quint16 deviceUdpPort);
    // 发送脱落检测开关指令
    void sendDetectionCommand(const QString &deviceHost, quint16 deviceUdpPort, bool enable);

    // === 数据访问(保持与旧接口兼容) ===
    // 从内部缓冲区取出一块数据 [通道数][采样点数]
    QVector<QVector<double>> getNextBlock(int samplesPerChannel);
    // 缓冲区中每个通道可用的采样点数
    int availableSamples() const;

    // === 信息查询 ===
    Config config() const { return m_config; }
    State state() const { return m_state; }
    int channelCount() const { return m_config.channelCount; }
    int sampleRate() const { return m_config.sampleRate; }
    QStringList channelLabels() const { return m_config.channelLabels; }
    quint16 listenPort() const { return m_config.listenPort; }

    // 弹出 TCP 服务端配置对话框，返回用户是否确认
    static bool showConfigDialog(quint16 &outPort, QString &outUdpHost,
                                  quint16 &outUdpPort, bool &outDetectionEnable, QWidget *parent);

signals:
    // 服务端已开始监听
    void serverStarted(quint16 port);
    // 设备已连接
    void clientConnected(const QString &address);
    // 设备断开
    void clientDisconnected();
    // 收到头部信息(保持兼容性, 在设备连接后立即发出)
    void headerReceived(int channelCount, int sampleRate, QStringList labels);
    // 有新数据到达
    void dataReady();
    // 连接状态变化
    void stateChanged(NetworkReceiver::State newState);
    // 错误信息
    void errorOccurred(const QString &message);
    // 缓冲区溢出警告
    void bufferWarning();

private slots:
    void onNewConnection();
    void onReadyRead();
    void onClientDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);

private:
    // 解析 108 字节数据包，提取 32 通道采样值
    void parsePackets(const QByteArray &data);
    // 解析单个 108 字节包
    bool parseOnePacket(const char *packet, QVector<double> &samples);
    void setState(State newState);

    // 默认 32 通道标准蒙太奇标签
    static QStringList defaultChannelLabels();

    QTcpServer *m_server = nullptr;
    QTcpSocket *m_clientSocket = nullptr;
    QUdpSocket *m_udpSocket = nullptr;
    Config m_config;
    State m_state = State::Disconnected;

    // 按通道组织的缓冲区
    QVector<QVector<double>> m_channelBuffers;
    int m_maxBufferSamples = 100000;  // ~50秒@2kHz, 防内存溢出
    QByteArray m_packetBuffer;        // TCP 粘包/半包缓存

    // 协议常量
    static constexpr int kPacketSize = 108;      // 一包总字节数
    static constexpr int kChannelsPerPacket = 32; // 每包通道数
    static constexpr int kBytesPerChannel = 3;    // 每通道字节数(24-bit)
    static constexpr quint8 kFrameHeader0 = 0xA1;
    static constexpr quint8 kFrameHeader1 = 0x05;

    // 统计
    QElapsedTimer m_statsTimer;
    qint64 m_bytesReceived = 0;
    qint64 m_samplesReceived = 0;
    quint32 m_lastSeqNum = 0;
    bool m_firstPacket = true;
    int m_lostPackets = 0;
};

#endif // NETWORKRECEIVER_H
