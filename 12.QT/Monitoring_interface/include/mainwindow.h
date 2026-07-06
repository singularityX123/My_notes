// mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include "bdfparser.h"
#include "networkreceiver.h"
#include "trialannotation.h"

struct ProcessingResult;  // 前向声明，完整定义在 eegprocessingworker.h
struct FbccaResult;       // 前向声明，完整定义在 ssvepfbcca.h

class ChartManager;
class PlaybackController;
class TopographyVisualizer;
class DataPipeline;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// ============================================================================
// MainWindow — 顶层协调器
// 职责：UI 组装、数据源切换（BDF/网络）、模块间信号桥接
// 具体的图表/回放/拓扑/管线逻辑已提取到独立类中
// ============================================================================
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 数据源
    void onOpenBDFFile();
    void onStartNetworkReceive();

    // 回放控制（转发到 PlaybackController）
    void onTogglePlayPause();

    // 网络事件
    void onNetworkHeaderReceived(int channelCount, int sampleRate, QStringList labels);
    void onNetworkStateChanged(NetworkReceiver::State state);

    // 定时器回调
    void onReplayTick();
    void onNetworkTick();

    // 数据管线结果
    void onWaveformUpdated();
    void onProcessingResultReady(const ProcessingResult &result);

    // 回放跳转
    void onSeekRequested(qint64 targetSample);

private:
    void setupUI();
    void setupShortcuts();
    void setupConnections();

    void resetPipelineAndVisuals();
    int  currentSampleRate() const;
    void updateTrialAnnotation();

    Ui::MainWindow *ui = nullptr;

    // === 子模块（按单一责任拆分） ===

    // 数据源
    BDFParser       *bdfParser       = nullptr;
    NetworkReceiver *networkReceiver  = nullptr;

    // 图表管理
    ChartManager    *chartManager     = nullptr;

    // 回放控制
    PlaybackController *playbackCtrl  = nullptr;

    // 拓扑图可视化
    TopographyVisualizer *topoVisualizer = nullptr;

    // 数据处理管线
    DataPipeline    *dataPipeline     = nullptr;

    // === UI 控件（setupUI 创建） ===
    QPushButton *startButton   = nullptr;
    QSlider     *playbackSlider = nullptr;
    QLabel      *playbackTimeLabel = nullptr;

    // === 定时器 ===
    QTimer *replayTimer  = nullptr;   // BDF 回放帧定时
    QTimer *networkTimer = nullptr;   // 网络数据消费定时

    // === 运行时状态 ===
    bool    isNetworkMode      = false;
    qint64  m_samplesReceived  = 0;
    int     blockSize          = 250;
    int     frameCounter       = 0;
    int     visualFrameDivisor = 1;

    // 数据源参数
    QVector<double> channelPositions;

    // SSVEP 刺激频率
    double leftStimulusFreq  = 10.0;
    double rightStimulusFreq = 12.0;

    // 试次标注
    TrialAnnotation trialAnnotator;
    bool  miInImaginationPhase = false;

    // BDF 文件路径（用于 CSV 关联）
    QString bdfFilePath;

    // UDP 设备控制（用于停止时发送停止传输指令）
    QString m_deviceUdpHost;
    quint16 m_deviceUdpPort = 0;
    bool m_detectionEnabled = false;
};

#endif // MAINWINDOW_H

