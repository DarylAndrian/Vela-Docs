// ════════════════════════════════════════════════════════════════════════════
//  LegacyDocBridge.cpp — Phase 4
// ════════════════════════════════════════════════════════════════════════════
#include "formats/LegacyDocBridge.h"
#include "util/Logger.h"

namespace vela {

LegacyDocBridge::LegacyDocBridge(QObject* parent)
    : QObject(parent)
{
}

bool LegacyDocBridge::convertToDocx(const QString& sourcePath,
                                     const QString& destDocxPath,
                                     QString& outError)
{
    Q_UNUSED(sourcePath);
    Q_UNUSED(destDocxPath);
    outError = QStringLiteral("LegacyDocBridge: Phase 4 will implement this (LibreOffice headless)");
    VE_LOG_WARNING(outError);
    return false;
}

bool LegacyDocBridge::convertFromDocx(const QString& sourceDocxPath,
                                       const QString& destPath,
                                       QString& outError)
{
    Q_UNUSED(sourceDocxPath);
    Q_UNUSED(destPath);
    outError = QStringLiteral("LegacyDocBridge: Phase 4 will implement this (LibreOffice headless)");
    VE_LOG_WARNING(outError);
    return false;
}

} // namespace vela
