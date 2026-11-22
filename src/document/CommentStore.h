// ════════════════════════════════════════════════════════════════════════════
//  CommentStore.h — Anchored comment threads (v0.3.3 — IMP-02)
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QUuid>
#include <QVector>

namespace vela {

class Document;

struct Comment
{
    QUuid     id           = QUuid::createUuid();
    QString   author       = QStringLiteral("me");
    QDateTime created      = QDateTime::currentDateTime();
    int       anchorFrom   = 0;
    int       anchorTo     = 0;
    QString   text;
    bool      resolved     = false;
    QVector<Comment> replies;
};

class CommentStore : public QObject
{
    Q_OBJECT
public:
    explicit CommentStore(QObject* parent = nullptr);

    /// All threads (top-level comments).
    const QVector<Comment>& threads() const { return m_threads; }

    /// Add a comment anchored to a character range.
    QUuid  addComment(const QString& text, int anchorFrom, int anchorTo);
    /// Reply to an existing thread.
    QUuid  replyTo(const QUuid& parentId, const QString& text);
    /// Resolve / reopen.
    bool   setResolved(const QUuid& id, bool resolved);
    /// Erase a comment (and its replies).
    bool   erase(const QUuid& id);

    /// Serialize to XML for the DOCX `<w:comments>` part.
    QByteArray serializeForDocx() const;

signals:
    void commentAdded(const QUuid& id);
    void commentResolved(const QUuid& id, bool resolved);
    void commentErased(const QUuid& id);

private:
    QVector<Comment> m_threads;

    bool findIn(QVector<Comment>& list, const QUuid& id, int* outIndex, QVector<Comment>** outParent);
};

} // namespace vela
