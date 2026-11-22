// CommentStore.cpp
#include "document/CommentStore.h"
#include <QXmlStreamWriter>

namespace vela {

CommentStore::CommentStore(QObject* parent) : QObject(parent) {}

QUuid CommentStore::addComment(const QString& text, int anchorFrom, int anchorTo)
{
    Comment c;
    c.anchorFrom = anchorFrom;
    c.anchorTo   = anchorTo;
    c.text       = text;
    m_threads.append(c);
    emit commentAdded(c.id);
    return c.id;
}

QUuid CommentStore::replyTo(const QUuid& parentId, const QString& text)
{
    int idx = 0;
    QVector<Comment>* parent = nullptr;
    if (!findIn(m_threads, parentId, &idx, &parent))
        return {};
    Comment reply;
    reply.text = text;
    (*parent)[idx].replies.append(reply);
    emit commentAdded(reply.id);
    return reply.id;
}

bool CommentStore::setResolved(const QUuid& id, bool resolved)
{
    int idx = 0;
    QVector<Comment>* parent = nullptr;
    if (!findIn(m_threads, id, &idx, &parent))
        return false;
    (*parent)[idx].resolved = resolved;
    emit commentResolved(id, resolved);
    return true;
}

bool CommentStore::erase(const QUuid& id)
{
    for (int i = 0; i < m_threads.size(); ++i)
        if (m_threads[i].id == id) { m_threads.removeAt(i); emit commentErased(id); return true; }
    for (auto& t : m_threads)
        for (int i = 0; i < t.replies.size(); ++i)
            if (t.replies[i].id == id) { t.replies.removeAt(i); emit commentErased(id); return true; }
    return false;
}

bool CommentStore::findIn(QVector<Comment>& list, const QUuid& id, int* outIndex, QVector<Comment>** outParent)
{
    for (int i = 0; i < list.size(); ++i)
        if (list[i].id == id) { *outIndex = i; *outParent = &list; return true; }
    for (auto& t : list)
        for (int i = 0; i < t.replies.size(); ++i)
            if (t.replies[i].id == id) { *outIndex = i; *outParent = &t.replies; return true; }
    return false;
}

QByteArray CommentStore::serializeForDocx() const
{
    QString out;
    QXmlStreamWriter w(&out);
    w.setAutoFormatting(false);
    w.writeStartElement(QStringLiteral("w:comments"));
    int id = 0;
    auto writeComment = [&](const Comment& c, const QString& parentRef) {
        w.writeStartElement(QStringLiteral("w:comment"));
        w.writeAttribute(QStringLiteral("w:id"),        QString::number(id++));
        w.writeAttribute(QStringLiteral("w:author"),    c.author);
        w.writeAttribute(QStringLiteral("w:date"),      c.created.toString(Qt::ISODate));
        if (!parentRef.isEmpty())
            w.writeAttribute(QStringLiteral("w:parent"),  parentRef);
        w.writeStartElement(QStringLiteral("w:p"));
        w.writeTextElement(QStringLiteral("w:t"), c.text);
        w.writeEndElement();
        w.writeEndElement();
    };
    for (const Comment& c : m_threads) {
        writeComment(c, QString());
        for (const Comment& r : c.replies) writeComment(r, c.id.toString());
    }
    w.writeEndElement();
    return out.toUtf8();
}

} // namespace vela
