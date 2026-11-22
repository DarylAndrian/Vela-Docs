// ════════════════════════════════════════════════════════════════════════════
//  RtfImporter.h — Phase 4
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>

namespace vela {

class RtfImporter : public QObject
{
    Q_OBJECT
public:
    explicit RtfImporter(QObject* parent = nullptr);
    bool importFromFile(const QString& path, QJsonObject& outBody, QString& outError);
};

} // namespace vela
