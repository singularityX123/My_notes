// ============================================================================
// wavewidget.cpp —— 见 wavewidget.h
// 深色示波器风格:自适应 Y 轴 + 网格 + 滚动曲线 + 最右数值标签
// ============================================================================
#include "wavewidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QtMath>

#include <algorithm>

WaveWidget::WaveWidget(QWidget *parent) : QWidget(parent) {
    setMinimumHeight(220);
    setAutoFillBackground(false);
    // 用独立刷新定时器驱动重绘(约 30fps),与数据到达解耦
    refresh_ = new QTimer(this);
    connect(refresh_, &QTimer::timeout, this,
            [this]() { if (data_.size()) update(); });
    refresh_->start(33);
}

void WaveWidget::setSampleRate(double fs) {
    fs_ = (fs > 0.0) ? fs : 2000.0;
    update();
}

void WaveWidget::setWindowSeconds(double s) {
    windowSec_ = std::max(0.5, s);
    update();
}

void WaveWidget::appendSamples(const QVector<double> &v) {
    const int maxPts = static_cast<int>(fs_ * windowSec_);
    if (maxPts <= 0) return;
    if (v.isEmpty()) return;
    data_ += v;
    if (data_.size() > maxPts)
        data_.remove(0, data_.size() - maxPts);  // 只保留最近一个时间窗
}

void WaveWidget::clear() { data_.clear(); update(); }

void WaveWidget::setBaseLine(double v) {
    baseLine_ = v;
    showBaseLine_ = true;
    update();
}

void WaveWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const double w = width(), h = height();
    const QRectF r(0, 0, w, h);
    p.fillRect(r, QColor(18, 22, 30));  // 背景

    if (data_.isEmpty()) {
        p.setPen(QColor(120, 130, 145));
        p.drawText(r, Qt::AlignCenter,
                   QStringLiteral("等待数据…(先启动 lsl_sim)"));
        return;
    }

    // ---- 纵轴自适应:取当前窗口内数据 min/max ----
    double vmin = 1e18, vmax = -1e18;
    for (double v : data_) {
        vmin = std::min(vmin, v);
        vmax = std::max(vmax, v);
    }
    if (showBaseLine_) { vmin = std::min(vmin, baseLine_); vmax = std::max(vmax, baseLine_); }
    if (vmax - vmin < 1e-6) { vmin -= 1.0; vmax += 1.0; }
    const double pad = (vmax - vmin) * 0.12;
    vmin -= pad; vmax += pad;

    auto yOf = [&](double v) {
        return h - (v - vmin) / (vmax - vmin) * h;
    };

    // ---- 网格 + 坐标 ----
    p.setPen(QPen(QColor(42, 50, 62), 1));
    const int ny = 5;
    for (int i = 0; i <= ny; ++i) {
        double y = h * i / ny;
        p.drawLine(QPointF(0, y), QPointF(w, y));
    }
    const int nx = 8;
    for (int i = 0; i <= nx; ++i) {
        double x = w * i / nx;
        p.drawLine(QPointF(x, 0), QPointF(x, h));
    }

    p.setFont(QFont("Monospace", 8));
    p.setPen(QColor(150, 160, 175));
    // 右上角显示当前值 / 左下标显示范围
    p.drawText(QRectF(6, 4, w - 12, 16), Qt::AlignLeft | Qt::AlignTop,
               QStringLiteral("max %1 µV").arg(vmax, 0, 'f', 1));
    p.drawText(QRectF(6, h - 18, w - 12, 14), Qt::AlignLeft | Qt::AlignBottom,
               QStringLiteral("min %1 µV").arg(vmin, 0, 'f', 1));

    // ---- 基线(可选) ----
    if (showBaseLine_) {
        p.setPen(QPen(QColor(90, 120, 90), 1, Qt::DashLine));
        p.drawLine(QPointF(0, yOf(baseLine_)), QPointF(w, yOf(baseLine_)));
    }

    // ---- 曲线 ----
    p.setPen(QPen(QColor(70, 200, 120), 1.6));
    const int n = data_.size();
    QPainterPath path;
    for (int i = 0; i < n; ++i) {
        double x = w * (i + 1) / n;
        double y = yOf(data_.at(i));
        if (i == 0)
            path.moveTo(x, y);
        else
            path.lineTo(x, y);
    }
    p.drawPath(path);
}
