// ════════════════════════════════════════════════════════════════════════════
//  OdtImporter.h — Phase 4
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>

namespace vela {

class OdtImporter : public QObject
{
    Q_OBJECT
public:
    explicit OdtImporter(QObject* parent = nullptr);
    bool importFromFile(const QString& path, QJsonObject& outBody, QString& outError);
};

} // namespace vela
