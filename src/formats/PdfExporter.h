// ════════════════════════════════════════════════════════════════════════════
//  PdfExporter.h — Phase 3
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>

namespace vela {

class PdfExporter : public QObject
{
    Q_OBJECT
public:
    explicit PdfExporter(QObject* parent = nullptr);
    bool exportToFile(const QString& path, const QJsonObject& body, QString& outError);
};

} // namespace vela
