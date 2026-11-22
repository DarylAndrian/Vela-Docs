// ════════════════════════════════════════════════════════════════════════════
//  MarkdownImporter.h — Phase 4 (uses cmark-gfm)
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>

namespace vela {

class MarkdownImporter : public QObject
{
    Q_OBJECT
public:
    explicit MarkdownImporter(QObject* parent = nullptr);
    bool importFromFile(const QString& path, QJsonObject& outBody, QString& outError);
    QString exportToString(const QJsonObject& body, QString& outError);
};

} // namespace vela
