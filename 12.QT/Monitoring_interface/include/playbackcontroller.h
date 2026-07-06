#ifndef PLAYBACKCONTROLLER_H
#define PLAYBACKCONTROLLER_H

#include <QObject>
#include <QElapsedTimer>
#include <QTimer>
#include <QSlider>
#include <QLabel>
#include <QPushButton>

// ============================================================================
// PlaybackController — 回放状态机 / 进度管理 / 跳转控制
// 单一责任：管理"播放/暂停/跳转"行为，不涉及数据读取或可视化
// ============================================================================

class PlaybackController : public QObject
{
    Q_OBJECT

public:
    explicit PlaybackController(QObject *parent = nullptr);

    // 绑定 UI 控件（由 MainWindow::setupUI 调用）
    void bindSlider(QSlider *slider);
    void bindTimeLabel(QLabel *label);
    void bindPlayButton(QPushButton *button);

    // 状态
    bool isPlaying() const { return m_playing; }

    // 设置数据源参数
    void setTotalSamples(qint64 total);
    void setSampleRate(int rate);
    void setCurrentSample(qint64 sample);  // 外部同步当前位置

    // 控制
    void toggle();               // 播放/暂停切换
    void play();
    void pause();
    void skipBySeconds(double seconds, qint64 currentPos);

    // 每帧调用：计算应读取的采样数（墙钟纠偏）
    // 返回应读取的采样数；0 表示还需等待；-1 表示无数据可读（未播放或文件结束）
    qint64 samplesToRead(qint64 currentPos);

    // 获取格式化时间字符串
    QString timeString(qint64 currentSample) const;

signals:
    void playStateChanged(bool playing);
    void seekRequested(qint64 targetSample);  // 用户拖动滑条/快进快退

private slots:
    void onSliderPressed();
    void onSliderReleased();
    void onSliderMoved(int value);

private:
    QString formatTime(double seconds) const;

    // UI 控件（不拥有所有权）
    QSlider     *m_slider    = nullptr;
    QLabel      *m_timeLabel = nullptr;
    QPushButton *m_playBtn   = nullptr;

    // 状态
    bool    m_playing            = false;
    bool    m_userDragging       = false;
    bool    m_baseSet            = false;  // play()后首次samplesToRead时锁存基准
    qint64  m_totalSamples       = 0;
    int     m_sampleRate         = 1;
    qint64  m_baseSample         = 0;     // 本次播放起始采样
    QElapsedTimer m_clock;
};

#endif // PLAYBACKCONTROLLER_H
