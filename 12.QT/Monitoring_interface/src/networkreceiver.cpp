// networkreceiver.cpp
#include "networkreceiver.h"
#include <QDebug>
#include <QHostAddress>
#include <QHostInfo>
#include <QtGlobal>

#if defined(Q_OS_UNIX)
#include <sys/types.h>
#include <sys/socket.h>
#endif

// ── helpers ──
namespace {

bool resolveHostAddress(const QString &host, QHostAddress &out)
{
    QHostAddress addr;
    if (addr.setAddress(host)) {
        out = addr;
        return true;
    }

    const QHostInfo info = QHostInfo::fromName(host);
    const auto addrs = info.addresses();
    for (const QHostAddress &a : addrs) {
        if (a.protocol() == QAbstractSocket::IPv4Protocol
            || a.protocol() == QAbstractSocket::IPv6Protocol) {
            out = a;
            return true;
        }
    }
    return false;
}

// 读取 24-bit 有符号整数 (little-endian)
qint32 readSigned24(const quint8 *ptr)
{
    qint32 value = static_cast<qint32>(ptr[0])
                 | (static_cast<qint32>(ptr[1]) << 8)
                 | (static_cast<qint32>(ptr[2]) << 16);
    // 符号扩展
    if (value & 0x800000) {
        value |= 0xFF000000;
    }
    return value;
}

// 读取 uint32 little-endian
quint32 readUint32LE(const quint8 *ptr)
{
    return static_cast<quint32>(ptr[0])
         | (static_cast<quint32>(ptr[1]) << 8)
         | (static_cast<quint32>(ptr[2]) << 16)
         | (static_cast<quint32>(ptr[3]) << 24);
}

// 默认 32 通道标准 10-20 蒙太奇标签
QStringList makeDefault32ChannelLabels()
{
    return {
        "Fp1", "Fp2",
        "F7", "F3", "Fz", "F4", "F8",
        "FC5", "FC1", "FC2", "FC6",
        "T7", "C3", "Cz", "C4", "T8",
        "CP5", "CP1", "CP2", "CP6",
        "P7", "P3", "Pz", "P4", "P8",
        "PO7", "PO3", "POz", "PO4", "PO8",
        "O1", "Oz", "O2"
    };
}

} // anonymous namespace

// ── NetworkReceiver ──
NetworkReceiver::NetworkReceiver(QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_clientSocket(nullptr)
    , m_udpSocket(new QUdpSocket(this))
{
    // UDP 发送端：绑定到任意 IPv4 地址的临时端口，并在 Unix 下显式开启广播权限。
    // 这样向 192.168.1.255 / 255.255.255.255 发送时更稳定。
    m_udpSocket->bind(QHostAddress::AnyIPv4, 0, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);

#if defined(Q_OS_UNIX)
    const int sd = m_udpSocket->socketDescriptor();
    if (sd >= 0) {
        int on = 1;
        ::setsockopt(sd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
    }
#endif

    connect(m_server, &QTcpServer::newConnection,
            this, &NetworkReceiver::onNewConnection);
}

NetworkReceiver::~NetworkReceiver()
{
    stopServer();
}

QStringList NetworkReceiver::defaultChannelLabels()
{
    return makeDefault32ChannelLabels();
}

// ── TCP 服务端 ──
void NetworkReceiver::startServer(quint16 port)
{
    stopServer();

    m_config.listenPort = port;
    m_config.channelCount = kChannelsPerPacket;
    m_config.sampleRate = 2000;
    m_config.channelLabels = makeDefault32ChannelLabels();

    // 初始化通道缓冲区
    m_channelBuffers.resize(kChannelsPerPacket);
    for (int ch = 0; ch < kChannelsPerPacket; ++ch) {
        m_channelBuffers[ch].reserve(m_maxBufferSamples / kChannelsPerPacket);
    }

    m_packetBuffer.clear();
    m_firstPacket = true;
    m_lostPackets = 0;
    m_bytesReceived = 0;
    m_samplesReceived = 0;
    m_statsTimer.start();

    if (!m_server->listen(QHostAddress::Any, port)) {
        setState(Error);
        emit errorOccurred(QString("无法监听端口 %1: %2")
                               .arg(port).arg(m_server->errorString()));
        return;
    }

    setState(Listening);
    emit serverStarted(port);

    qDebug() << "[NetworkReceiver] TCP 服务端已在端口" << port << "监听, 等待脑电设备连接...";
}

void NetworkReceiver::stopServer()
{
    if (m_clientSocket) {
        m_clientSocket->disconnectFromHost();
        if (m_clientSocket->state() != QAbstractSocket::UnconnectedState) {
            m_clientSocket->waitForDisconnected(500);
        }
        m_clientSocket->deleteLater();
        m_clientSocket = nullptr;
    }

    m_server->close();
    m_channelBuffers.clear();
    m_packetBuffer.clear();
    m_firstPacket = true;

    setState(Disconnected);
}

// ── UDP 控制指令 ──
void NetworkReceiver::sendStartCommand(const QString &deviceHost, quint16 deviceUdpPort)
{
    const QByteArray cmd = QByteArray::fromHex("BB6601");

    QHostAddress addr;
    if (!resolveHostAddress(deviceHost, addr)) {
        qDebug() << "[NetworkReceiver] 发送开始传输指令失败：无法解析地址 ->" << deviceHost;
        return;
    }

    const qint64 sent = m_udpSocket->writeDatagram(cmd, addr, deviceUdpPort);
    qDebug() << "[NetworkReceiver] 发送开始传输指令 ->" << deviceHost << ":" << deviceUdpPort
             << (sent == cmd.size() ? "成功" : "失败")
             << "(sent=" << sent << ")";
}

void NetworkReceiver::sendDetectionCommand(const QString &deviceHost, quint16 deviceUdpPort, bool enable)
{
    const QByteArray cmd = enable ? QByteArray::fromHex("BB6500") : QByteArray::fromHex("BB6501");

    QHostAddress addr;
    if (!resolveHostAddress(deviceHost, addr)) {
        qDebug() << "[NetworkReceiver] 发送脱落检测指令失败：无法解析地址 ->" << deviceHost;
        return;
    }

    const qint64 sent = m_udpSocket->writeDatagram(cmd, addr, deviceUdpPort);
    qDebug() << "[NetworkReceiver] 发送脱落检测" << (enable ? "开启" : "关闭")
             << "->" << deviceHost << ":" << deviceUdpPort
             << (sent == cmd.size() ? "成功" : "失败")
             << "(sent=" << sent << ")";
}

// ── 数据访问 ──
QVector<QVector<double>> NetworkReceiver::getNextBlock(int samplesPerChannel)
{
    QVector<QVector<double>> result;
    if (m_channelBuffers.isEmpty() || samplesPerChannel <= 0) {
        return result;
    }

    const int channels = m_channelBuffers.size();
    const int available = availableSamples();
    const int toRead = qMin(samplesPerChannel, available);
    if (toRead <= 0) {
        return result;
    }

    result.resize(channels);
    for (int ch = 0; ch < channels; ++ch) {
        QVector<double> &buf = m_channelBuffers[ch];
        result[ch] = buf.mid(0, toRead);
        buf.remove(0, toRead);
    }
    return result;
}

int NetworkReceiver::availableSamples() const
{
    if (m_channelBuffers.isEmpty()) {
        return 0;
    }
    return m_channelBuffers.first().size();
}

// ── Internal slots ──
void NetworkReceiver::onNewConnection()
{
    // 只接受一个设备连接（单客户端）
    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();
        if (!socket) continue;

        // 如果已有连接，拒绝新的
        if (m_clientSocket) {
            qDebug() << "[NetworkReceiver] 已有设备连接，拒绝新连接:" << socket->peerAddress().toString();
            socket->disconnectFromHost();
            socket->deleteLater();
            continue;
        }

        m_clientSocket = socket;
        connect(m_clientSocket, &QTcpSocket::readyRead,
                this, &NetworkReceiver::onReadyRead);
        connect(m_clientSocket, &QTcpSocket::disconnected,
                this, &NetworkReceiver::onClientDisconnected);
        connect(m_clientSocket, &QTcpSocket::errorOccurred,
                this, &NetworkReceiver::onSocketError);

        // 设备连接后立即发出 headerReceived，让 MainWindow 初始化
        m_firstPacket = true;
        m_lostPackets = 0;
        m_packetBuffer.clear();

        setState(Connected);
        const QString addr = m_clientSocket->peerAddress().toString();
        emit clientConnected(addr);
        emit headerReceived(kChannelsPerPacket, 2000, makeDefault32ChannelLabels());

        qDebug() << "[NetworkReceiver] 脑电设备已连接:" << addr;
    }
}

void NetworkReceiver::onReadyRead()
{
    if (!m_clientSocket) return;

    const QByteArray data = m_clientSocket->readAll();
    if (data.isEmpty()) return;

    m_bytesReceived += data.size();
    m_packetBuffer.append(data);

    parsePackets(m_packetBuffer);
}

void NetworkReceiver::onClientDisconnected()
{
    qDebug() << "[NetworkReceiver] 脑电设备断开连接";
    if (m_clientSocket) {
        m_clientSocket->deleteLater();
        m_clientSocket = nullptr;
    }
    m_packetBuffer.clear();
    setState(Listening);
    emit clientDisconnected();
}

void NetworkReceiver::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    if (m_clientSocket) {
        qDebug() << "[NetworkReceiver] Socket 错误:" << m_clientSocket->errorString();
        emit errorOccurred(m_clientSocket->errorString());
    }
}

// ── 数据包解析 ──
void NetworkReceiver::parsePackets(const QByteArray &data)
{
    if (data.size() < kPacketSize) {
        return;
    }

    const quint8 *raw = reinterpret_cast<const quint8 *>(data.constData());
    const int totalBytes = data.size();

    int consumed = 0;
    const int channels = kChannelsPerPacket;

    while (consumed + kPacketSize <= totalBytes) {
        const quint8 *pkt = raw + consumed;

        // 验证帧头 0xA1 0x05
        if (pkt[0] != kFrameHeader0 || pkt[1] != kFrameHeader1) {
            qDebug() << "[NetworkReceiver] 帧头不匹配:" << Qt::hex << pkt[0] << pkt[1]
                     << "，同步中...";
            // 帧头不匹配，跳过 1 字节继续搜索
            consumed += 1;
            continue;
        }

        // 跳过: 帧头(2) + 电池(1) + 脱落检测(5) = 8
        // 包序号: 4 bytes (offset 8)
        const quint32 seqNum = readUint32LE(pkt + 8);

        // 检测丢包
        if (!m_firstPacket) {
            const quint32 expected = m_lastSeqNum + 1;
            if (seqNum != expected) {
                const int lost = static_cast<int>(seqNum - expected);
                if (lost > 0 && lost < 1000) { // 合理范围内才计数
                    m_lostPackets += lost;
                    qDebug() << "[NetworkReceiver] ⚠ 丢包: 期望" << expected
                             << "实际" << seqNum << "(丢失" << lost << "包, 累计" << m_lostPackets << ")";
                }
            }
        }
        m_lastSeqNum = seqNum;
        m_firstPacket = false;

        // 32 通道数据: offset 12, 每通道 3 字节 (signed 24-bit)
        const int dataOffset = 12; // 2 + 1 + 5 + 4
        const quint8 *chanData = pkt + dataOffset;

        // 检查缓冲区是否会溢出
        if (availableSamples() + 1 > m_maxBufferSamples) {
            emit bufferWarning();
            for (int ch = 0; ch < channels; ++ch) {
                const int drop = m_channelBuffers[ch].size() / 2;
                if (drop > 0) {
                    m_channelBuffers[ch].remove(0, drop);
                }
            }
        }

        for (int ch = 0; ch < channels; ++ch) {
            const qint32 rawValue = readSigned24(chanData + ch * kBytesPerChannel);
            // 直接作为 double 存储，后续可通过增益/偏移校准
            m_channelBuffers[ch].append(static_cast<double>(rawValue));
        }

        m_samplesReceived += 1;
        consumed += kPacketSize;
    }

    // 保留未消费的尾部数据
    if (consumed > 0) {
        m_packetBuffer.remove(0, consumed);
    }

    if (m_samplesReceived > 0) {
        emit dataReady();
    }
}

bool NetworkReceiver::parseOnePacket(const char *packet, QVector<double> &samples)
{
    const quint8 *pkt = reinterpret_cast<const quint8 *>(packet);
    if (pkt[0] != kFrameHeader0 || pkt[1] != kFrameHeader1) {
        return false;
    }

    samples.resize(kChannelsPerPacket);
    const quint8 *chanData = pkt + 12;
    for (int ch = 0; ch < kChannelsPerPacket; ++ch) {
        samples[ch] = static_cast<double>(readSigned24(chanData + ch * kBytesPerChannel));
    }
    return true;
}

#include <QMessageBox>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>

bool NetworkReceiver::showConfigDialog(quint16 &outPort, QString &outUdpHost,
                                        quint16 &outUdpPort, bool &outDetectionEnable, QWidget *parent)
{
    QDialog dlg(parent);
    dlg.setWindowTitle("TCP 服务端配置");
    dlg.setMinimumWidth(420);

    auto *layout = new QVBoxLayout(&dlg);
    layout->setSpacing(12);
    layout->setContentsMargins(20, 16, 20, 16);

    auto *title = new QLabel("本机作为 TCP 服务端，等待脑电设备(CC3235)连接:");
    title->setStyleSheet("font-size: 13px; font-weight: bold;");
    layout->addWidget(title);

    auto *portLayout = new QHBoxLayout();
    portLayout->addWidget(new QLabel("监听端口:"));
    auto *portEdit = new QLineEdit("5001");
    portLayout->addWidget(portEdit);
    layout->addLayout(portLayout);

    auto *udpLabel = new QLabel("UDP 设备控制（可选）:");
    udpLabel->setStyleSheet("font-size: 12px; color: #555;");
    layout->addWidget(udpLabel);

    auto *udpIpLay = new QHBoxLayout();
    udpIpLay->addWidget(new QLabel("目标地址(IP/广播):"));
    auto *udpIpEdit = new QLineEdit("192.168.1.255");
    udpIpLay->addWidget(udpIpEdit);
    layout->addLayout(udpIpLay);

    auto *udpPortLay = new QHBoxLayout();
    udpPortLay->addWidget(new QLabel("UDP 端口:"));
    auto *udpPortEdit = new QLineEdit("5001");
    udpPortLay->addWidget(udpPortEdit);
    layout->addLayout(udpPortLay);

    auto *detectLay = new QHBoxLayout();
    auto *detectCheck = new QCheckBox("开启脱落检测", &dlg);
    detectCheck->setChecked(false);
    detectLay->addWidget(detectCheck);
    detectLay->addStretch(1);
    layout->addLayout(detectLay);

    auto *info = new QLabel(
        "协议说明:\n"
        " 1) 脑电设备(CC3235)作为 TCP 客户端主动连接本机\n"
        " 2) 108 字节数据包: 0xA1 0x05 + 12字节头部 + 32通道x3字节\n"
        " 3) 采样率: 2000Hz, 通道数: 32\n"
        " 4) 可选: 通过 UDP(可广播)发送 0xBB 0x66 0x01 开始传输\n"
        " 5) 可选: 通过 UDP 发送 0xBB 0x65 0x00/0x01 开启/关闭脱落检测");
    info->setWordWrap(true);
    info->setStyleSheet("color: #666; font-size: 11px; background: #f5f5f5; "
                        "border: 1px solid #ddd; border-radius: 4px; padding: 8px;");
    layout->addWidget(info);

    auto *btnLay = new QHBoxLayout();
    auto *startBtn = new QPushButton("启动监听");
    auto *cancelBtn = new QPushButton("取消");
    btnLay->addStretch();
    btnLay->addWidget(cancelBtn);
    btnLay->addWidget(startBtn);
    layout->addLayout(btnLay);

    QObject::connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    QObject::connect(startBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    if (dlg.exec() != QDialog::Accepted)
        return false;

    bool portOk = false;
    outPort = static_cast<quint16>(portEdit->text().trimmed().toUShort(&portOk));
    if (!portOk || outPort == 0) {
        QMessageBox::warning(parent, "输入错误", "请输入有效的监听端口号");
        return false;
    }
    outUdpHost = udpIpEdit->text().trimmed();
    bool udpPortOk = false;
    outUdpPort = static_cast<quint16>(udpPortEdit->text().trimmed().toUShort(&udpPortOk));
    if (!udpPortOk) outUdpPort = 5001;

    outDetectionEnable = detectCheck->isChecked();
    return true;
}

void NetworkReceiver::setState(State newState)
{
    if (m_state != newState) {
        m_state = newState;
        emit stateChanged(newState);
    }
}
