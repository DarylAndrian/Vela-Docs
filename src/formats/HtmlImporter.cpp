#include "formats/HtmlImporter.h"
#include "util/Logger.h"

namespace vela {

HtmlImporter::HtmlImporter(QObject* parent) : QObject(parent) {}

bool HtmlImporter::importFromString(const QString& html, QJsonObject& outBody, QString& outError)
{
    Q_UNUSED(html);
    Q_UNUSED(outBody);
    outError = QStringLiteral("HtmlImporter: Phase 4 (parses to ProseMirror schema).");
    VE_LOG_WARNING(outError);
    return false;
}

bool HtmlImporter::importFromFile(const QString& path, QJsonObject& outBody, QString& outError)
{
    Q_UNUSED(path);
    Q_UNUSED(outBody);
    outError = QStringLiteral("HtmlImporter: Phase 4.");
    VE_LOG_WARNING(outError);
    return false;
}

} // namespace vela
