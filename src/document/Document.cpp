// ════════════════════════════════════════════════════════════════════════════
//  Document.cpp
// ════════════════════════════════════════════════════════════════════════════
#include "document/Document.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonValue>
#include <QRegularExpression>

namespace vela {

Document::Document(QObject* parent)
    : QObject(parent)
    , m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
}

Document::~Document() = default;

void Document::setFilePath(const QString& path)
{
    if (m_filePath == path) return;
    m_filePath = path;
    emit filePathChanged(m_filePath);
}

QString Document::title() const
{
    if (m_filePath.isEmpty()) {
        return QStringLiteral("Untitled");
    }
    return QFileInfo(m_filePath).fileName();
}

void Document::setDirty(bool d)
{
    if (m_dirty == d) return;
    m_dirty = d;
    emit dirtyChanged(m_dirty);
}

void Document::setBody(const QJsonObject& body)
{
    if (m_body == body) return;
    m_body = body;
    emit bodyChanged();
}

QString Document::plainText() const
{
    // Quick recursive extraction of all "text" fields in the JSON tree.
    // Good enough for Phase 0 word counting; Phase 2 will hook into the
    // real editor model for live counts.
    QString acc;
    std::function<void(const QJsonValue&)> walk = [&](const QJsonValue& v) {
        if (v.isString()) {
            acc += v.toString();
            acc += QLatin1Char(' ');
        } else if (v.isObject()) {
            const QJsonObject o = v.toObject();
            if (o.contains(QStringLiteral("text"))) {
                acc += o.value(QStringLiteral("text")).toString();
                acc += QLatin1Char(' ');
            }
            for (auto it = o.begin(); it != o.end(); ++it) {
                walk(*it);
            }
        } else if (v.isArray()) {
            for (const QJsonValue& child : v.toArray()) {
                walk(child);
            }
        }
    };
    walk(QJsonValue(m_body));

    // Collapse whitespace
    static const QRegularExpression ws(QStringLiteral("\\s+"));
    acc.replace(ws, QStringLiteral(" "));
    return acc.trimmed();
}

} // namespace vela
