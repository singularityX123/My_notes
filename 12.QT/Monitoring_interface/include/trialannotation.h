#ifndef TRIALANNOTATION_H
#define TRIALANNOTATION_H

#include <QVector>
#include <QString>
#include <QDateTime>

// ============================================================================
// TrialAnnotation — CSV试次标签解析 + 想象期状态机
// ============================================================================

struct TrialEvent {
    int trial = 0;
    QString condition;
    QString direction;
    QString eventType;
    double startRelativeTime = 0.0;
    double alignedRelativeTime = 0.0;
    QDateTime eventTime;
    bool hasAlignedRelativeTime = false;
};

class TrialAnnotation {
public:
    // 从 CSV 文件加载试次事件
    bool loadCsv(const QString &filePath,
                 const QDateTime &recordingStartTime);

    // 清空
    void clear();

    // 查询
    bool isLoaded() const { return m_loaded; }
    int  eventCount() const { return m_events.size(); }
    const QVector<TrialEvent> &events() const { return m_events; }

    // 根据当前时间更新内部状态
    // 返回：当前应显示的 MI 意图文本
    //  outIsImagining: 是否处于想象期
    QString update(double currentTimeSec, double totalDurationSec,
                   bool &outIsImagining);

private:
    QVector<TrialEvent> m_events;
    bool m_loaded = false;
};

#endif
