// ════════════════════════════════════════════════════════════════════════════
//  Document.h — In-memory document model
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QUuid>

namespace vela {

/**
 * @brief The Document class holds the in-memory representation of one open
 *        document. In Phase 0 it's a thin wrapper around a JSON payload;
 *        Phase 1 replaces it with a ProseMirror-compatible doc model.
 *
 * Owned by DocumentController; documents are shared via QSharedPointer so
 * multiple views can reference the same doc safely.
 */
class Document : public QObject
{
    Q_OBJECT
public:
    explicit Document(QObject* parent = nullptr);
    ~Document() override;

    /// Stable identifier (UUIDv4).
    QString id() const { return m_id; }

    /// File path on disk (empty if untitled).
    QString filePath() const { return m_filePath; }
    void setFilePath(const QString& path);

    /// Human-readable title (file name or "Untitled").
    QString title() const;

    /// Has the document been modified since last save?
    bool isDirty() const { return m_dirty; }
    void setDirty(bool d);

    /// The body content (format depends on phase):
    /// - Phase 0: HTML snippet
    /// - Phase 1+: ProseMirror JSON
    QJsonObject body() const { return m_body; }
    void setBody(const QJsonObject& body);

    /// Plain-text extraction (for word counts, search, etc.).
    QString plainText() const;

signals:
    void dirtyChanged(bool dirty);
    void filePathChanged(const QString& path);
    void bodyChanged();

private:
    QString     m_id;
    QString     m_filePath;
    QJsonObject m_body;
    bool        m_dirty = false;
};

} // namespace vela
