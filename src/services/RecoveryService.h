// ════════════════════════════════════════════════════════════════════════════
//  RecoveryService.h — Crash recovery at launch (v0.3.1 — CRIT-03)
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QUuid>

#include <vector>

namespace vela {

/**
 * @brief Snapshot of a recoverable autosave file found on disk.
 */
struct RecoverableDocument
{
    QUuid     id;
    QString   autosavePath;
    QString   originalPath;   ///< last known user path, if any
    QDateTime saved;
    qint64    sizeBytes = 0;
};

class MainWindow; // forward

/**
 * @brief Scans for orphaned autosaves on launch and lets the MainWindow show
 * the recovery dialog. Triggered exactly once per app start.
 */
class RecoveryService : public QObject
{
    Q_OBJECT
public:
    explicit RecoveryService(QObject* parent = nullptr);

    /// Look for autosaves on disk.
    std::vector<RecoverableDocument> scan();
    /// True if any autosaves would be offered to the user.
    bool hasRecoverables();

    /// Recover a single autosave back into a temporary file the user can save.
    /// Returns the path on disk (likely in Temp/) the caller should open.
    QString recover(const RecoverableDocument& doc);

    /// Wipe an autosave file (called after successful user save).
    static bool discard(const RecoverableDocument& doc);

signals:
    void recoverablesFound(const std::vector<RecoverableDocument>& docs);

private:
    QString autosaveDir() const;
};

} // namespace vela
