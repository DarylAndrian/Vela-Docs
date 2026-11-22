// ════════════════════════════════════════════════════════════════════════════
//  AutoSaver.cpp
// ════════════════════════════════════════════════════════════════════════════
#include "services/AutoSaver.h"

#include "document/TabSession.h"
#include "document/Document.h"
#include "document/DocumentController.h"
#include "formats/DocxExporter.h"
#include "services/RevisionStore.h"
#include "services/SettingsStore.h"
#include "util/Logger.h"
#include "util/PathUtils.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QUuid>

namespace vela {

AutoSaver::AutoSaver(QObject* parent)
    : QObject(parent)
{
    // Pull settings overrides on first construction.
    SettingsStore settings;
    m_intervalSec         = settings.value(QStringLiteral("autoSave/intervalSec"), 30).toInt();
    m_revisionIntervalMin = settings.value(QStringLiteral("autoSave/revisionMin"), 5).toInt();

    m_timer.setSingleShot(false);
    connect(&m_timer, &QTimer::timeout, this, &AutoSaver::tick);
}

AutoSaver::~AutoSaver()
{
    stop();
}

void AutoSaver::start()
{
    m_timer.start(m_intervalSec * 1000);
}

void AutoSaver::stop()
{
    m_timer.stop();
}

void AutoSaver::setIntervalSeconds(int s)
{
    if (s < 5)  s = 5;
    if (s > 600) s = 600;
    m_intervalSec = s;
    if (m_timer.isActive())
        m_timer.start(m_intervalSec * 1000);
    SettingsStore settings;
    settings.setValue(QStringLiteral("autoSave/intervalSec"), m_intervalSec);
}

void AutoSaver::registerSession(TabSession* session)
{
    if (!session)
        return;
    for (auto* s : m_sessions)
        if (s == session)
            return;
    m_sessions.push_back(session);
}

void AutoSaver::unregisterSession(TabSession* session)
{
    auto it = std::remove(m_sessions.begin(), m_sessions.end(), session);
    m_sessions.erase(it, m_sessions.end());
}

int AutoSaver::flushNow()
{
    int saved = 0;
    for (TabSession* s : m_sessions) {
        if (!s || !s->isDirty()) continue;
        const QString path = autosavePathFor(s);
        DocxExporter exporter;
        if (exporter.exportTo(s->document(), path)) {
            emit saved(s, path);
            ++saved;
        }
    }
    return saved;
}

void AutoSaver::tick()
{
    // Autosave every tick.
    flushNow();

    // Revision snapshot — every N minutes.
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastRevisionEpochMs >= qint64(m_revisionIntervalMin) * 60 * 1000) {
        m_lastRevisionEpochMs = now;
        for (TabSession* s : m_sessions)
            snapshotRevision(s);
    }
}

QString AutoSaver::autosavePathFor(const TabSession* session) const
{
    const QString dir = PathUtils::autosaveDir();
    QDir().mkpath(dir);
    return QStringLiteral("%1/%2.docx").arg(dir, session->id().toString(QUuid::WithoutBraces));
}

void AutoSaver::snapshotRevision(TabSession* session)
{
    if (!session) return;

    RevisionStore store;
    const QDateTime ts = QDateTime::currentDateTime();
    const QString preview = QStringLiteral("Autosave of %1 at %2")
                              .arg(session->title())
                              .arg(ts.toString(Qt::ISODate));
    // store.addRevision takes the docx bytes — emit then mark.
    const QString path = autosavePathFor(session);
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return;
    const QByteArray bytes = f.readAll();
    f.close();

    if (store.addRevision(session->id().toString(QUuid::WithoutBraces),
                          bytes, preview, ts))
        emit revisionSnapshotted(session);
    else
        Logger::warning(QStringLiteral("AutoSaver: snapshot failed for %1").arg(session->title()));
}

} // namespace vela
