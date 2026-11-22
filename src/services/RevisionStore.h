// ════════════════════════════════════════════════════════════════════════════
//  RevisionStore.h — Local revision history backed by SQLite
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVector>
#include <QSqlDatabase>

namespace vela {

/**
 * @brief Snapshot-based local revision history.
 *
 * Phase 0: schema only, no snapshots taken (Phase 5).
 * Each revision is a (timestamp, documentId, snapshotPath, summary) tuple.
 */
class RevisionStore : public QObject
{
    Q_OBJECT
public:
    struct Revision {
        qint64     id = 0;
        QString    documentId;
        QDateTime  timestamp;
        QString    summary;
        QString    snapshotPath; // relative path inside the revisions dir
        qint64     sizeBytes = 0;
    };

    explicit RevisionStore(const QString& dbPath, QObject* parent = nullptr);
    ~RevisionStore() override;

    bool isOpen() const;

    /// Take a snapshot of a document (full content).
    /// Phase 5: implementation. Phase 0: no-op.
    qint64 snapshot(const QString& documentId,
                    const QByteArray& content,
                    const QString& summary = {});

    /// List revisions for a document (newest first).
    QVector<Revision> listForDocument(const QString& documentId, int maxCount = 50) const;

    /// Get the raw bytes of a snapshot.
    QByteArray readSnapshot(qint64 revisionId) const;

signals:
    void revisionAdded(qint64 revisionId, const QString& documentId);

private:
    bool ensureSchema();

    QString       m_dbPath;
    QSqlDatabase  m_db;
    bool          m_open = false;
};

} // namespace vela
