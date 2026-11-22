#include "formats/RtfImporter.h"
#include "util/Logger.h"

namespace vela {

RtfImporter::RtfImporter(QObject* parent) : QObject(parent) {}

bool RtfImporter::importFromFile(const QString& path, QJsonObject& outBody, QString& outError)
{
    Q_UNUSED(path);
    Q_UNUSED(outBody);
    outError = QStringLiteral("RtfImporter: Phase 4 (uses LegacyDocBridge).");
    VE_LOG_WARNING(outError);
    return false;
}

} // namespace vela
