// ════════════════════════════════════════════════════════════════════════════
//  HtmlImporter.h — Phase 4
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>

namespace vela {

class HtmlImporter : public QObject
{
    Q_OBJECT
public:
    explicit HtmlImporter(QObject* parent = nullptr);
    bool importFromString(const QString& html, QJsonObject& outBody, QString& outError);
    bool importFromFile(const QString& path, QJsonObject& outBody, QString& outError);
};

} // namespace vela
