// ════════════════════════════════════════════════════════════════════════════
//  AutoSaver.h — Periodic autosave + revision snapshots (v0.3.1 — CRIT-03)
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QObject>
#include <QTimer>

#include <functional>
#include <vector>

namespace vela {

class TabSession;

/**
 * @brief Periodic autosave for every open TabSession.
 *
 * Autosaves dirty tabs to `QStandardPaths::AppDataLocation/autosave/<uuid>.docx`
 * on a configurable tick (default 30 s). On the same cadence (less frequent),
 * snapshots the Document into the existing `RevisionStore` (SQLite).
 *
 * The autosave is decoupled from the editor's transaction stream — it copies
 * the document state to a separate file, never modifies the live document.
 */
class AutoSaver : public QObject
{
    Q_OBJECT
public:
    explicit AutoSaver(QObject* parent = nullptr);
    ~AutoSaver() override;

    /// Start the autosave loop.
    void start();
    /// Stop the autosave loop.
    void stop();
    bool isRunning() const { return m_timer.isActive(); }

    /// Register a session so it gets autosaved.
    void registerSession(TabSession* session);
    /// Unregister a session (typically on tab close).
    void unregisterSession(TabSession* session);
    /// All registered sessions.
    const std::vector<TabSession*>& sessions() const { return m_sessions; }

    /// Seconds between autosave sweeps (default 30).
    void  setIntervalSeconds(int s);
    int   intervalSeconds() const { return m_intervalSec; }
    /// Minutes between revision snapshots (default 5).
    void  setRevisionIntervalMinutes(int m) { m_revisionIntervalMin = m; }
    int   revisionIntervalMinutes() const { return m_revisionIntervalMin; }

    /// One-shot autosave (used during shutdown to flush state).
    int   flushNow();        ///< returns count of sessions saved

signals:
    void saved(TabSession* session, const QString& path);
    void saveFailed(TabSession* session, const QString& reason);
    void revisionSnapshotted(TabSession* session);

private slots:
    void tick();

private:
    QString autosavePathFor(const TabSession* session) const;
    void    snapshotRevision(TabSession* session);

    QTimer                  m_timer;
    std::vector<TabSession*> m_sessions;
    int                     m_intervalSec            = 30;
    int                     m_revisionIntervalMin    = 5;
    qint64                  m_lastRevisionEpochMs    = 0;
};

} // namespace vela
