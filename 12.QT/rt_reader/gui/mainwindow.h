// ============================================================================
// mainwindow.h —— rt_reader Qt GUI 主窗口
// 顶部:启动/停止 + 流名过滤 + 通道选择 + 统计;中央:实时波形;底部:日志
// ============================================================================
#pragma once

#include <QMainWindow>
#include <QVector>

class QLabel;
class QPushButton;
class QLineEdit;
class QComboBox;
class WaveWidget;
class LslReceiver;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onStart();
    void onStop();
    void onStreamFound(const QString &name, const QString &type,
                       int channels, double srate);
    void onSamples(const QVector<double> &mux, int samples);
    void onStats(double rate, double totalSec, long long totalSamples);
    void onError(const QString &msg);
    void onChannelChanged(int index);

private:
    void setupUi();
    LslReceiver *receiver_ = nullptr;
    WaveWidget *wave_ = nullptr;

    QPushButton *startBtn_ = nullptr;
    QPushButton *stopBtn_ = nullptr;
    QLineEdit *nameEdit_ = nullptr;
    QComboBox *channelCombo_ = nullptr;
    QLabel *infoLabel_ = nullptr;
    QLabel *statsLabel_ = nullptr;
    QLabel *statusLabel_ = nullptr;

    int channels_ = 0;
    double fs_ = 2000.0;
    long long total_ = 0;
};
