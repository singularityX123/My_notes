#include "trialannotation.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QMessageBox>
#include <algorithm>

static QDateTime parseEventDateTime(const QString &text)
{
    QDateTime dt = QDateTime::fromString(text, "yyyy-MM-dd HH:mm:ss.zzz");
    if (dt.isValid()) return dt;
    int dot = text.indexOf('.');
    if (dot < 0) return QDateTime::fromString(text, "yyyy-MM-dd HH:mm:ss");
    QDateTime base = QDateTime::fromString(text.left(dot).trimmed(), "yyyy-MM-dd HH:mm:ss");
    if (!base.isValid()) return {};
    bool ok; int ms = text.mid(dot+1).trimmed().left(3).toInt(&ok);
    return ok ? base.addMSecs(ms) : base;
}

bool TrialAnnotation::loadCsv(const QString &filePath,
                               const QDateTime &recordingStartTime)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    QVector<TrialEvent> ev;
    QTextStream s(&f);
    bool hdr=false;

    while (!s.atEnd()) {
        QString line = s.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('=')) continue;
        if (!hdr) { hdr=true; if (line.startsWith("trial,")) continue; }

        QStringList cols = line.split(',', Qt::KeepEmptyParts);
        if (cols.size() < 6) continue;

        bool ok; int trial = cols[0].trimmed().toInt(&ok); if (!ok) continue;

        TrialEvent e; e.trial = trial;
        e.condition = cols[1].trimmed(); e.direction = cols[2].trimmed();
        e.eventType  = cols[3].trimmed();
        e.startRelativeTime = cols[4].trimmed().toDouble(&ok); if (!ok) continue;
        e.eventTime = parseEventDateTime(cols[5].trimmed());

        if (recordingStartTime.isValid() && e.eventTime.isValid()) {
            e.alignedRelativeTime = recordingStartTime.msecsTo(e.eventTime) / 1000.0;
            e.hasAlignedRelativeTime = true;
        }
        ev.append(e);
    }

    std::sort(ev.begin(), ev.end(), [](const TrialEvent &a, const TrialEvent &b) {
        if (a.trial != b.trial) return a.trial < b.trial;
        double ta = a.hasAlignedRelativeTime ? a.alignedRelativeTime : a.startRelativeTime;
        double tb = b.hasAlignedRelativeTime ? b.alignedRelativeTime : b.startRelativeTime;
        return ta < tb;
    });

    m_events = ev;
    m_loaded = !ev.isEmpty();
    return m_loaded;
}

void TrialAnnotation::clear() { m_events.clear(); m_loaded = false; }

QString TrialAnnotation::update(double currentTime, double totalDuration,
                                 bool &outIsImagining)
{
    outIsImagining = false;
    if (!m_loaded || m_events.isEmpty()) return "MI意图: 等待想象...";

    const TrialEvent *active = nullptr;
    bool imagining = false;

    for (int i = 0; i < m_events.size(); ++i) {
        const TrialEvent &e = m_events[i];
        double t = e.startRelativeTime;
        if (e.hasAlignedRelativeTime && totalDuration > 0.0) {
            bool inRange = (e.alignedRelativeTime >= -1.0) &&
                           (e.alignedRelativeTime <= totalDuration + 1.0);
            if (inRange) t = e.alignedRelativeTime;
        }
        if (t > currentTime) break;

        QString type = e.eventType.trimmed().toLower();
        if (type == "start_preparation" || type == "start_cue" || type == "start_imagination")
            active = &e;

        if (type == "start_imagination") imagining = true;
        else if (type == "end_imagination" && active && e.trial == active->trial) imagining = false;
        else if (type == "end_trial" && active && e.trial == active->trial)
            { active = nullptr; imagining = false; }
    }

    outIsImagining = imagining;

    if (!active) return "MI意图: 等待想象...";

    QString dir = active->direction.trimmed();
    if (dir.compare("left", Qt::CaseInsensitive) == 0) dir = "左";
    else if (dir.compare("right", Qt::CaseInsensitive) == 0) dir = "右";
    else dir = "未知";

    if (imagining) return QString("MI意图: %1").arg(dir);
    return "MI意图: 等待想象...";
}
