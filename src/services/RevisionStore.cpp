// ════════════════════════════════════════════════════════════════════════════
//  RevisionStore.cpp
// ════════════════════════════════════════════════════════════════════════════
#include "services/RevisionStore.h"
#include "util/Logger.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>

namespace vela {

namespace {
    constexpr const char* kConnectionName = "vela_revisions";
}

RevisionStore::RevisionStore(const QString& dbPath, QObject* parent)
    : QObject(parent)
    , m_dbPath(dbPath)
{
    QFileInfo info(dbPath);
    QDir().mkpath(info.absolutePath());

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QLatin1String(kConnectionName));
    m_db.setDatabaseName(dbPath);
    if (!m_db.open()) {
        VE_LOG_ERROR("Could not open revisions DB: " << m_db.lastError().text());
        m_open = false;
        return;
    }
    m_open = ensureSchema();
    if (m_open) {
        VE_LOG_INFO("RevisionStore opened: " << dbPath);
    }
}

RevisionStore::~RevisionStore()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
    QSqlDatabase::removeDatabase(QLatin1String(kConnectionName));
}

bool RevisionStore::isOpen() const { return m_open; }

bool RevisionStore::ensureSchema()
{
    QSqlQuery q(m_db);
    const QString create = QStringLiteral(R"SQL(
        CREATE TABLE IF NOT EXISTS revisions (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            document_id     TEXT NOT NULL,
            timestamp       DATETIME DEFAULT CURRENT_TIMESTAMP,
            summary         TEXT,
            snapshot_path   TEXT NOT NULL,
            size_bytes      INTEGER NOT NULL DEFAULT 0
        );
        CREATE INDEX IF NOT EXISTS idx_revisions_doc
            ON revisions(document_id, timestamp DESC);
    )SQL");
    if (!q.exec(create)) {
        VE_LOG_ERROR("RevisionStore schema error: " << q.lastError().text());
        return false;
    }
    return true;
}

qint64 RevisionStore::snapshot(const QString& documentId,
                                const QByteArray& content,
                                const QString& summary)
{
    Q_UNUSED(documentId);
    Q_UNUSED(content);
    Q_UNUSED(summary);
    // Phase 0: stub. Phase 5 will write content to disk and insert row.
    VE_LOG_DEBUG("RevisionStore::snapshot — Phase 5 implementation");
    return -1;
}

QVector<RevisionStore::Revision>
RevisionStore::listForDocument(const QString& documentId, int maxCount) const
{
    QVector<Revision> result;
    if (!m_open) return result;

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT id, document_id, timestamp, summary, snapshot_path, size_bytes "
        "FROM revisions WHERE document_id = :doc ORDER BY timestamp DESC LIMIT :n"));
    q.bindValue(QStringLiteral(":doc"), documentId);
    q.bindValue(QStringLiteral(":n"), maxCount);
    if (!q.exec()) {
        VE_LOG_WARNING("RevisionStore listForDocument error: " << q.lastError().text());
        return result;
    }
    while (q.next()) {
        Revision r;
        r.id            = q.value(0).toLongLong();
        r.documentId    = q.value(1).toString();
        r.timestamp     = q.value(2).toDateTime();
        r.summary       = q.value(3).toString();
        r.snapshotPath  = q.value(4).toString();
        r.sizeBytes     = q.value(5).toLongLong();
        result.append(r);
    }
    return result;
}

QByteArray RevisionStore::readSnapshot(qint64 revisionId) const
{
    QByteArray result;
    if (!m_open) return result;
    // Phase 5: read file by revision's snapshot_path
    Q_UNUSED(revisionId);
    return result;
}

} // namespace vela
