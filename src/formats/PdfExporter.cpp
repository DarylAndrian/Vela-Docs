#include "formats/PdfExporter.h"
#include "util/Logger.h"

namespace vela {

PdfExporter::PdfExporter(QObject* parent) : QObject(parent) {}

bool PdfExporter::exportToFile(const QString& path, const QJsonObject& body, QString& outError)
{
    Q_UNUSED(path);
    Q_UNUSED(body);
    outError = QStringLiteral("PdfExporter: Phase 3 (uses Chromium print or Qt Pdf).");
    VE_LOG_WARNING(outError);
    return false;
}

} // namespace vela
