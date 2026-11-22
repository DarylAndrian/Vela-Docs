#include "formats/OdtImporter.h"
#include "util/Logger.h"

namespace vela {

OdtImporter::OdtImporter(QObject* parent) : QObject(parent) {}

bool OdtImporter::importFromFile(const QString& path, QJsonObject& outBody, QString& outError)
{
    Q_UNUSED(path);
    Q_UNUSED(outBody);
    outError = QStringLiteral("OdtImporter: Phase 4 (uses LegacyDocBridge).");
    VE_LOG_WARNING(outError);
    return false;
}

} // namespace vela
