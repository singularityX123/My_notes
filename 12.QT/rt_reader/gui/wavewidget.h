// ============================================================================
// wavewidget.h —— 简易实时滚动波形(QPainter 自绘,无第三方依赖)
// ============================================================================
#pragma once

#include <QWidget>
#include <QVector>

class QTimer;

class WaveWidget : public QWidget {
    Q_OBJECT
public:
    explicit WaveWidget(QWidget *parent = nullptr);

    void setSampleRate(double fs);   // 横轴换算用(默认 2000)
    void setWindowSeconds(double s); // 显示多长的时间窗(默认 5s)
    void appendSamples(const QVector<double> &v);  // 追加新采样点(会自动裁剪)
    void clear();
    void setBaseLine(double v);      // 显示一条参考线/基线(可选)
    int sampleCount() const { return data_.size(); }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QVector<double> data_;
    double fs_ = 2000.0;
    double windowSec_ = 5.0;
    double baseLine_ = 0.0;
    bool showBaseLine_ = false;
    QTimer *refresh_ = nullptr;
};
