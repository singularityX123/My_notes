#include "playbackcontroller.h"
#include <algorithm>

PlaybackController::PlaybackController(QObject *parent)
    : QObject(parent)
{
}

void PlaybackController::bindSlider(QSlider *slider)
{
    m_slider = slider;
    if (m_slider) {
        connect(m_slider, &QSlider::sliderPressed,  this, &PlaybackController::onSliderPressed);
        connect(m_slider, &QSlider::sliderReleased, this, &PlaybackController::onSliderReleased);
        connect(m_slider, &QSlider::sliderMoved,    this, &PlaybackController::onSliderMoved);
    }
}

void PlaybackController::bindTimeLabel(QLabel *label)
{
    m_timeLabel = label;
}

void PlaybackController::bindPlayButton(QPushButton *button)
{
    m_playBtn = button;
}

void PlaybackController::setTotalSamples(qint64 total)
{
    m_totalSamples = total;
    if (m_slider)
        m_slider->setRange(0, qMax<qint64>(1, total));
}

void PlaybackController::setSampleRate(int rate)
{
    m_sampleRate = qMax(1, rate);
}

void PlaybackController::setCurrentSample(qint64 sample)
{
    if (m_slider && !m_userDragging)
        m_slider->setValue(static_cast<int>(sample));

    if (m_timeLabel) {
        m_timeLabel->setText(timeString(sample));
    }
}

void PlaybackController::toggle()
{
    if (m_playing)
        pause();
    else
        play();
}

void PlaybackController::play()
{
    m_playing = true;
    m_baseSet = false;
    m_clock.start();
    if (m_playBtn)
        m_playBtn->setText("暂停");
    emit playStateChanged(true);
}

void PlaybackController::pause()
{
    m_playing = false;
    m_clock.invalidate();
    if (m_playBtn)
        m_playBtn->setText("开始");
    emit playStateChanged(false);
}

void PlaybackController::skipBySeconds(double seconds, qint64 currentPos)
{
    if (m_sampleRate <= 0)
        return;

    const qint64 offset = static_cast<qint64>(seconds * m_sampleRate);
    const qint64 target = qBound<qint64>(0, currentPos + offset, m_totalSamples - 1);

    if (target != currentPos)
        emit seekRequested(target);
}

qint64 PlaybackController::samplesToRead(qint64 currentPos)
{
    if (!m_playing || m_sampleRate <= 0)
        return 0;

    if (!m_clock.isValid())
        return 0;

    // 首次调用时锁存当前播放位置作为基准，确保暂停后恢复能正确计算增量
    if (!m_baseSet) {
        m_baseSample = currentPos;
        m_baseSet = true;
    }

    const qint64 targetSample = m_baseSample +
                                static_cast<qint64>((m_clock.elapsed() * m_sampleRate) / 1000.0);
    const qint64 lag = targetSample - currentPos;

    if (lag <= 0)
        return 0;  // 超前，等待

    const qint64 maxCatchup = qMax<qint64>(1, m_sampleRate / 5);
    return qBound<qint64>(1, lag, maxCatchup);
}

QString PlaybackController::timeString(qint64 currentSample) const
{
    if (m_sampleRate <= 0)
        return "00:00.0 / 00:00.0";

    const double curSec  = static_cast<double>(currentSample) / m_sampleRate;
    const double totSec  = static_cast<double>(m_totalSamples) / m_sampleRate;

    return QString("%1 / %2")
        .arg(formatTime(curSec))
        .arg(formatTime(totSec));
}

QString PlaybackController::formatTime(double seconds) const
{
    const int mins = static_cast<int>(seconds) / 60;
    const double secs = seconds - mins * 60;
    return QString("%1:%2.%3")
        .arg(mins, 2, 10, QChar('0'))
        .arg(static_cast<int>(secs), 2, 10, QChar('0'))
        .arg(static_cast<int>(secs * 10) % 10);
}

// --- Slots ---

void PlaybackController::onSliderPressed()
{
    m_userDragging = true;
}

void PlaybackController::onSliderReleased()
{
    m_userDragging = false;
    if (m_slider)
        emit seekRequested(static_cast<qint64>(m_slider->value()));
}

void PlaybackController::onSliderMoved(int value)
{
    // sliderMoved 仅在用户拖动时触发，直接请求跳转
    if (value >= 0 && value < m_totalSamples)
        emit seekRequested(static_cast<qint64>(value));
}
