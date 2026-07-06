// mainwindow.cpp — 顶层协调器（最小化）
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "chartmanager.h"
#include "playbackcontroller.h"
#include "topographyvisualizer.h"
#include "datapipeline.h"
#include "electrodepositions.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , bdfParser(new BDFParser(this))
    , networkReceiver(new NetworkReceiver(this))
    , replayTimer(new QTimer(this))
    , networkTimer(new QTimer(this))
{
    ui->setupUi(this);

    chartManager    = new ChartManager(this);
    playbackCtrl    = new PlaybackController(this);
    dataPipeline    = new DataPipeline(this);
    topoVisualizer  = new TopographyVisualizer(ui->topographyView, this);

    setupUI();
    setupShortcuts();
    setupConnections();
}

MainWindow::~MainWindow()
{
    if (networkTimer->isActive())   networkTimer->stop();
    if (replayTimer->isActive())    replayTimer->stop();
    if (networkReceiver) {
        if (!m_deviceUdpHost.isEmpty() && m_deviceUdpPort != 0 && m_detectionEnabled)
            networkReceiver->sendDetectionCommand(m_deviceUdpHost, m_deviceUdpPort, false);
        networkReceiver->stopServer();
    }
    delete ui;
}

// ========== UI 装配 ==========

void MainWindow::setupUI()
{
    setWindowTitle("混合范式脑电解码可视化平台");

    auto *mainGrid = qobject_cast<QGridLayout*>(ui->centralwidget->layout());
    if (mainGrid) {
        mainGrid->setRowStretch(0, 3);
        mainGrid->setRowStretch(1, 2);
        mainGrid->setRowStretch(2, 4);
    }

    QWidget *controlPanel = new QWidget(this);
    auto *panelLayout = new QVBoxLayout(controlPanel);
    panelLayout->setContentsMargins(8, 4, 8, 4);
    panelLayout->setSpacing(6);

    playbackSlider = new QSlider(Qt::Horizontal, controlPanel);
    playbackSlider->setRange(0, 1);
    playbackSlider->setValue(0);
    playbackSlider->setTracking(true);

    auto *buttonRow = new QHBoxLayout();
    auto *openBtn    = new QPushButton("处理BDF文件数据", controlPanel);
    auto *netBtn     = new QPushButton("处理实时网络数据", controlPanel);
    startButton      = new QPushButton("开始", controlPanel);
    playbackTimeLabel = new QLabel("00:00.0 / 00:00.0", controlPanel);

    buttonRow->addWidget(openBtn);
    buttonRow->addWidget(netBtn);
    buttonRow->addStretch(1);
    buttonRow->addWidget(startButton);
    buttonRow->addStretch(1);
    buttonRow->addWidget(playbackTimeLabel);

    panelLayout->addWidget(playbackSlider);
    panelLayout->addLayout(buttonRow);

    if (mainGrid) {
        mainGrid->addWidget(controlPanel, 3, 0, 1, 2);
        mainGrid->setRowStretch(3, 1);
    }

    playbackCtrl->bindSlider(playbackSlider);
    playbackCtrl->bindTimeLabel(playbackTimeLabel);
    playbackCtrl->bindPlayButton(startButton);

    connect(openBtn, &QPushButton::clicked, this, &MainWindow::onOpenBDFFile);
    connect(netBtn,  &QPushButton::clicked, this, &MainWindow::onStartNetworkReceive);
    connect(startButton, &QPushButton::clicked, this, &MainWindow::onTogglePlayPause);
}

void MainWindow::setupShortcuts()
{
    auto addKey = [this](auto key, auto fn) {
        auto *sc = new QShortcut(key, this);
        connect(sc, &QShortcut::activated, this, fn);
    };
    addKey(Qt::Key_Space,  &MainWindow::onTogglePlayPause);
    addKey(Qt::Key_Left,   [this]{ playbackCtrl->skipBySeconds(-1.0,  bdfParser->getCurrentPosition()); });
    addKey(Qt::Key_Right,  [this]{ playbackCtrl->skipBySeconds( 1.0,  bdfParser->getCurrentPosition()); });
    addKey(Qt::Key_Down,   [this]{ playbackCtrl->skipBySeconds(-10.0, bdfParser->getCurrentPosition()); });
    addKey(Qt::Key_Up,     [this]{ playbackCtrl->skipBySeconds( 10.0, bdfParser->getCurrentPosition()); });
    addKey(QKeySequence(Qt::CTRL | Qt::Key_O), &MainWindow::onOpenBDFFile);
}

void MainWindow::setupConnections()
{
    chartManager->initSpectrumChart(ui->spectrumLayout);
    chartManager->initEEGChart(ui->eegLayout);

    replayTimer->setInterval(33);   replayTimer->setTimerType(Qt::PreciseTimer);
    networkTimer->setInterval(33);   networkTimer->setTimerType(Qt::PreciseTimer);
    connect(replayTimer,  &QTimer::timeout, this, &MainWindow::onReplayTick);
    connect(networkTimer, &QTimer::timeout, this, &MainWindow::onNetworkTick);

    connect(networkReceiver, &NetworkReceiver::headerReceived,   this, &MainWindow::onNetworkHeaderReceived);
    connect(networkReceiver, &NetworkReceiver::stateChanged,     this, &MainWindow::onNetworkStateChanged);
    connect(networkReceiver, &NetworkReceiver::errorOccurred,    this, [this](const QString &s) { statusBar()->showMessage("网络错误: " + s); });

    connect(playbackCtrl, &PlaybackController::seekRequested,    this, &MainWindow::onSeekRequested);

    connect(dataPipeline, &DataPipeline::waveformUpdated,        this, &MainWindow::onWaveformUpdated);
    connect(dataPipeline, &DataPipeline::processingResultReady,  this, &MainWindow::onProcessingResultReady);
}

// ========== 数据源切换 ==========

void MainWindow::resetPipelineAndVisuals()
{
    dataPipeline->reset();
    topoVisualizer->resetBaseline();
    chartManager->clearSpectrum();
    chartManager->clearEEG();
    miInImaginationPhase = false;
}

int MainWindow::currentSampleRate() const
{
    if (isNetworkMode && networkReceiver) return networkReceiver->sampleRate();
    if (bdfParser) return bdfParser->getHeader().sampleRate;
    return 0;
}

void MainWindow::onOpenBDFFile()
{
    if (isNetworkMode) {
        if (networkTimer->isActive()) networkTimer->stop();
        if (networkReceiver && !m_deviceUdpHost.isEmpty() && m_deviceUdpPort != 0 && m_detectionEnabled)
            networkReceiver->sendDetectionCommand(m_deviceUdpHost, m_deviceUdpPort, false);
        networkReceiver->stopServer();
        isNetworkMode = false;
        m_samplesReceived = 0;
    }

    const QString filePath = QFileDialog::getOpenFileName(this, "打开BDF文件", "", "BDF文件 (*.bdf)");
    if (filePath.isEmpty()) return;

    if (!bdfParser->openFile(filePath)) {
        QMessageBox::critical(this, "错误", "无法解析BDF文件");
        return;
    }

    if (replayTimer->isActive()) replayTimer->stop();
    playbackCtrl->pause();
    bdfFilePath = filePath;
    resetPipelineAndVisuals();

    const BDFHeader header = bdfParser->getHeader();
    channelPositions = ElectrodePositions::fromLabels(header.channelLabels, 128);
    dataPipeline->initChannelMapping(header.channelLabels);
    dataPipeline->setChannelInfo(channelPositions, header.channelLabels);

    blockSize = qMax(1, header.sampleRate / 30);
    playbackCtrl->setTotalSamples(header.numSamples);
    playbackCtrl->setSampleRate(header.sampleRate);
    playbackCtrl->setCurrentSample(bdfParser->getCurrentPosition());

    statusBar()->showMessage(QString("已加载: %1 | 采样率: %2 Hz | 通道数: %3")
        .arg(header.patientID).arg(header.sampleRate).arg(header.numChannels));
    if (header.startDate.isValid())
        statusBar()->showMessage(statusBar()->currentMessage() +
            QString(" | 记录开始: %1").arg(header.startDate.toString("yyyy-MM-dd HH:mm:ss.zzz")));

    const QString csvPath = QFileDialog::getOpenFileName(this, "打开对应CSV标签文件",
        QFileInfo(filePath).absolutePath() + "/" + QFileInfo(filePath).completeBaseName() + ".csv",
        "CSV文件 (*.csv)");

    if (!csvPath.isEmpty()) {
        if (!trialAnnotator.loadCsv(csvPath, header.startDate))
            QMessageBox::warning(this, "提示", "CSV已选择，但未能解析任何有效标签事件");
        else
            statusBar()->showMessage(statusBar()->currentMessage() +
                QString(" | 事件数: %1").arg(trialAnnotator.eventCount()));
    } else {
        trialAnnotator.clear();
    }
}

void MainWindow::onStartNetworkReceive()
{
    // 停止当前网络连接
    if (isNetworkMode && networkReceiver
        && networkReceiver->state() != NetworkReceiver::Disconnected)
    {
        if (networkTimer->isActive()) networkTimer->stop();
        if (replayTimer->isActive())  replayTimer->stop();
        playbackCtrl->pause();

        if (!m_deviceUdpHost.isEmpty() && m_deviceUdpPort != 0 && m_detectionEnabled)
            networkReceiver->sendDetectionCommand(m_deviceUdpHost, m_deviceUdpPort, false);

        networkReceiver->stopServer();
        isNetworkMode = false;
        statusBar()->showMessage("TCP 服务端已停止");
        return;
    }

    quint16 listenPort, udpPort;
    QString udpHost;
    bool detectionEnable = false;
    if (!NetworkReceiver::showConfigDialog(listenPort, udpHost, udpPort, detectionEnable, this))
        return;

    m_deviceUdpHost = udpHost;
    m_deviceUdpPort = udpPort;
    m_detectionEnabled = detectionEnable;

    if (replayTimer->isActive()) replayTimer->stop();
    playbackCtrl->pause();

    isNetworkMode = true;
    m_samplesReceived = 0;
    resetPipelineAndVisuals();

    statusBar()->showMessage(QString("正在端口 %1 启动监听...").arg(listenPort));
    networkReceiver->startServer(listenPort);

    if (!m_deviceUdpHost.isEmpty() && m_deviceUdpPort != 0) {
        networkReceiver->sendStartCommand(m_deviceUdpHost, m_deviceUdpPort);
        if (m_detectionEnabled)
            networkReceiver->sendDetectionCommand(m_deviceUdpHost, m_deviceUdpPort, true);
    }
}

void MainWindow::onNetworkHeaderReceived(int, int sampleRate, QStringList labels)
{
    channelPositions = ElectrodePositions::fromLabels(labels, 128);
    blockSize = qMax(1, sampleRate / 30);
    dataPipeline->initChannelMapping(labels);
    dataPipeline->setChannelInfo(channelPositions, labels);

    statusBar()->showMessage(QString("设备已连接 | 采样率: %1 Hz | 监听端口: %2")
        .arg(sampleRate).arg(networkReceiver->listenPort()));

    if (!networkTimer->isActive()) networkTimer->start();
    playbackCtrl->play();
}

void MainWindow::onNetworkStateChanged(NetworkReceiver::State state)
{
    switch (state) {
    case NetworkReceiver::Disconnected:
        if (isNetworkMode) { if (networkTimer->isActive()) networkTimer->stop(); playbackCtrl->pause(); statusBar()->showMessage("TCP 服务端已停止"); }
        break;
    case NetworkReceiver::Listening:
        statusBar()->showMessage(QString("TCP 服务端监听中 (端口 %1)，等待设备连接...").arg(networkReceiver->listenPort()));
        break;
    case NetworkReceiver::Error:
        if (networkTimer->isActive()) networkTimer->stop();
        playbackCtrl->pause(); isNetworkMode = false;
        statusBar()->showMessage("TCP 服务端启动失败");
        break;
    default: break;
    }
}

// ========== 播放/暂停/跳转 ==========

void MainWindow::onTogglePlayPause()
{
    if (playbackCtrl->isPlaying()) {
        if (replayTimer->isActive())  replayTimer->stop();
        if (networkTimer->isActive()) networkTimer->stop();
        playbackCtrl->pause();
        statusBar()->showMessage("已暂停");
        return;
    }

    if (isNetworkMode && networkReceiver
        && networkReceiver->state() == NetworkReceiver::Connected) {
        playbackCtrl->play();
        if (!networkTimer->isActive()) networkTimer->start();
        statusBar()->showMessage("实时接收已恢复");
        return;
    }

    if (!bdfParser || bdfParser->getHeader().numChannels <= 0) {
        QMessageBox::warning(this, "提示", "请先打开有效的BDF文件或连接网络数据源");
        return;
    }

    if (blockSize <= 0) blockSize = qMax(1, bdfParser->getHeader().sampleRate / 30);
    playbackCtrl->play();
    playbackCtrl->setTotalSamples(bdfParser->getHeader().numSamples);
    playbackCtrl->setSampleRate(bdfParser->getHeader().sampleRate);

    if (!replayTimer->isActive()) replayTimer->start();
    statusBar()->showMessage("已启动可视化");
}

void MainWindow::onSeekRequested(qint64 targetSample)
{
    if (!bdfParser) return;
    const bool wasPlaying = playbackCtrl->isPlaying();
    if (wasPlaying) { if (replayTimer->isActive()) replayTimer->stop(); playbackCtrl->pause(); }
    if (!bdfParser->seekToSample(targetSample)) { if (wasPlaying) onTogglePlayPause(); return; }

    resetPipelineAndVisuals();
    playbackCtrl->setCurrentSample(bdfParser->getCurrentPosition());
    updateTrialAnnotation();
    if (wasPlaying) onTogglePlayPause();
}

// ========== 帧处理 ==========

void MainWindow::updateTrialAnnotation()
{
    if (!trialAnnotator.isLoaded()) return;
    const int sr = currentSampleRate();
    if (sr <= 0) return;
    qint64 pos = bdfParser ? bdfParser->getCurrentPosition() : m_samplesReceived;
    ui->miIntentLabel->setText(trialAnnotator.update(
        static_cast<double>(pos) / sr,
        bdfParser ? static_cast<double>(bdfParser->getHeader().numSamples) / sr : 0.0,
        miInImaginationPhase));
    dataPipeline->setInImagination(miInImaginationPhase);
}

void MainWindow::onReplayTick()
{
    if (!playbackCtrl->isPlaying() || !bdfParser) return;
    ++frameCounter;

    const BDFHeader &h = bdfParser->getHeader();
    const int sr = qMax(1, h.sampleRate);
    const qint64 pos = bdfParser->getCurrentPosition();
    qint64 toRead = playbackCtrl->samplesToRead(pos);
    if (toRead <= 0) { playbackCtrl->setCurrentSample(pos); updateTrialAnnotation(); return; }

    toRead = qBound<qint64>(1, toRead, static_cast<qint64>(blockSize));
    QVector<QVector<double>> block = bdfParser->getNextBlock(static_cast<int>(toRead));
    if (block.isEmpty() || block[0].isEmpty()) { playbackCtrl->pause(); if (replayTimer->isActive()) replayTimer->stop(); statusBar()->showMessage("播放结束"); return; }

    dataPipeline->processBlock(block, sr, (frameCounter % qMax(1, visualFrameDivisor)) == 0);
    playbackCtrl->setCurrentSample(bdfParser->getCurrentPosition());
    updateTrialAnnotation();
}

void MainWindow::onNetworkTick()
{
    if (!isNetworkMode || !networkReceiver) return;
    if (!playbackCtrl->isPlaying() || networkReceiver->state() != NetworkReceiver::Connected) return;

    ++frameCounter;
    const int sr = qMax(1, networkReceiver->sampleRate());
    QVector<QVector<double>> block = networkReceiver->getNextBlock(qMax(1, blockSize));
    if (block.isEmpty() || block[0].isEmpty()) return;

    m_samplesReceived += block[0].size();
    dataPipeline->processBlock(block, sr, (frameCounter % qMax(1, visualFrameDivisor)) == 0);

    if (playbackTimeLabel && sr > 0) {
        const double elapsed = static_cast<double>(m_samplesReceived) / sr;
        playbackTimeLabel->setText(QString("实时 %1:%2.%3")
            .arg(static_cast<int>(elapsed) / 60, 2, 10, QChar('0'))
            .arg(static_cast<int>(elapsed) % 60, 2, 10, QChar('0'))
            .arg(static_cast<int>(elapsed * 10) % 10));
    }
}

// ========== 可视化回调 ==========

void MainWindow::onWaveformUpdated()
{
    chartManager->updateEEGWaveform(dataPipeline->waveformBuffer(), currentSampleRate());
}

void MainWindow::onProcessingResultReady(const ProcessingResult &result)
{
    // MI 热力图：全程动态显示
    if (!result.topomapImage.isNull()) {
        QVector<QString> labels;
        if (bdfParser) labels = bdfParser->getHeader().channelLabels;
        else if (isNetworkMode && networkReceiver) labels = networkReceiver->channelLabels();
        topoVisualizer->update(result.topomapImage, channelPositions, labels);
    }

    // SSVEP 状态：全程实时显示；注视方向：仅想象期显示
    chartManager->updateSSVEPUI(
        ui->leftStatusLabel, ui->rightStatusLabel,
        ui->gazeDirectionLabel,
        result.fbcca, leftStimulusFreq, rightStimulusFreq);
    if (!miInImaginationPhase && ui->gazeDirectionLabel)
        ui->gazeDirectionLabel->clear();

    chartManager->updateSpectrum(result.psd, static_cast<double>(currentSampleRate()));
}
