// ============================================================================
// mainwindow.cpp —— 见 mainwindow.h
// ============================================================================
#include "mainwindow.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "lsl_receiver.h"
#include "wavewidget.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUi();
    receiver_ = new LslReceiver(this);
    connect(receiver_, &LslReceiver::streamFound, this,
            &MainWindow::onStreamFound);
    connect(receiver_, &LslReceiver::samplesBatch, this,
            &MainWindow::onSamples);
    connect(receiver_, &LslReceiver::statsUpdated, this,
            &MainWindow::onStats);
    connect(receiver_, &LslReceiver::errorOccurred, this,
            &MainWindow::onError);
    connect(receiver_, &LslReceiver::logMessage, this,
            [this](const QString &m) { statusLabel_->setText(m); });

    stopBtn_->setEnabled(false);
    wave_->setSampleRate(fs_);
}

MainWindow::~MainWindow() {
    if (receiver_ && receiver_->isRunning()) {
        receiver_->stop();
        receiver_->wait(2000);
    }
}

void MainWindow::setupUi() {
    setWindowTitle(QStringLiteral("rt_reader — LSL 拉流实时波形"));
    resize(900, 520);

    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);

    // ---- 顶栏控制 ----
    auto *ctl = new QHBoxLayout;
    startBtn_ = new QPushButton(QStringLiteral("开始接收"), central);
    stopBtn_ = new QPushButton(QStringLiteral("停止"), central);
    nameEdit_ = new QLineEdit(central);
    nameEdit_->setPlaceholderText(
        QStringLiteral("流名过滤(留空=自动找 type 对应流)"));
    nameEdit_->setFixedWidth(230);
    channelCombo_ = new QComboBox(central);
    channelCombo_->setEnabled(false);

    ctl->addWidget(startBtn_);
    ctl->addWidget(stopBtn_);
    ctl->addWidget(new QLabel(QStringLiteral("流名:"), central));
    ctl->addWidget(nameEdit_);
    ctl->addWidget(new QLabel(QStringLiteral("通道:"), central));
    ctl->addWidget(channelCombo_);
    ctl->addStretch();
    root->addLayout(ctl);

    // ---- 信息/统计 ----
    infoLabel_ = new QLabel(QStringLiteral("未连接"), central);
    statsLabel_ = new QLabel(QStringLiteral("0 样本"), central);
    auto *bar = new QHBoxLayout;
    bar->addWidget(infoLabel_);
    bar->addStretch();
    bar->addWidget(statsLabel_);
    root->addLayout(bar);

    // ---- 波形 ----
    wave_ = new WaveWidget(central);
    root->addWidget(wave_, 1);

    // ---- 底部状态 ----
    statusLabel_ = new QLabel(QStringLiteral("就绪:先启动 lsl_sim 再点“开始接收”"),
                              central);
    root->addWidget(statusLabel_);

    setCentralWidget(central);

    connect(startBtn_, &QPushButton::clicked, this, &MainWindow::onStart);
    connect(stopBtn_, &QPushButton::clicked, this, &MainWindow::onStop);
    connect(channelCombo_, &QComboBox::currentIndexChanged, this,
            &MainWindow::onChannelChanged);
}

void MainWindow::onStart() {
    if (receiver_->isRunning()) return;
    wave_->clear();
    total_ = 0;
    startBtn_->setEnabled(false);
    stopBtn_->setEnabled(true);
    statusLabel_->setText(QStringLiteral("正在解析流…"));
    receiver_->configure(QStringLiteral("EEG"), nameEdit_->text().trimmed());
    receiver_->start();
}

void MainWindow::onStop() {
    receiver_->stop();
    statusLabel_->setText(QStringLiteral("已请求停止…"));
    startBtn_->setEnabled(true);
    stopBtn_->setEnabled(false);
}

void MainWindow::onStreamFound(const QString &name, const QString &type,
                               int channels, double srate) {
    channels_ = channels;
    fs_ = srate;
    wave_->setSampleRate(fs_);
    infoLabel_->setText(QStringLiteral("流: %1 | type=%2 | ch=%3 | %4 Hz")
                            .arg(name, type)
                            .arg(channels)
                            .arg(srate, 0, 'f', 0));
    channelCombo_->clear();
    channelCombo_->setEnabled(channels > 1);
    for (int i = 0; i < channels; ++i)
        channelCombo_->addItem(QStringLiteral("通道 %1").arg(i + 1));
    channelCombo_->setCurrentIndex(0);
}

void MainWindow::onChannelChanged(int index) {
    (void)index;
    wave_->clear();  // 切换通道后从当前时刻重新画
}

void MainWindow::onSamples(const QVector<double> &mux, int samples) {
    if (channels_ <= 0 || mux.isEmpty()) return;
    const int c = channelCombo_->currentIndex();
    // 从交错缓冲中抽出当前所选通道的样本
    QVector<double> pts;
    pts.reserve(samples);
    const int nch = channels_;
    for (int s = 0; s < samples; ++s)
        pts.append(mux.at(static_cast<int>(s) * nch + c));
    wave_->appendSamples(pts);
}

void MainWindow::onStats(double rate, double totalSec, long long totalSamples) {
    total_ = totalSamples;
    statsLabel_->setText(QStringLiteral("%1 样本 | %2 样本/s | 运行 %3 s")
                             .arg(totalSamples)
                             .arg(rate, 0, 'f', 0)
                             .arg(totalSec, 0, 'f', 1));
}

void MainWindow::onError(const QString &msg) {
    statusLabel_->setText(QStringLiteral("错误"));
    startBtn_->setEnabled(true);
    stopBtn_->setEnabled(false);
    QMessageBox::warning(this, QStringLiteral("LSL 接收错误"), msg);
}
