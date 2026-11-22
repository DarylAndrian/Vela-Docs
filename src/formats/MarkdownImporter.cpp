#include "formats/MarkdownImporter.h"
#include "util/Logger.h"

namespace vela {

MarkdownImporter::MarkdownImporter(QObject* parent) : QObject(parent) {}

bool MarkdownImporter::importFromFile(const QString& path, QJsonObject& outBody, QString& outError)
{
    Q_UNUSED(path);
    Q_UNUSED(outBody);
    outError = QStringLiteral("MarkdownImporter: Phase 4 (uses cmark-gfm).");
    VE_LOG_WARNING(outError);
    return false;
}

QString MarkdownImporter::exportToString(const QJsonObject& body, QString& outError)
{
    Q_UNUSED(body);
    outError = QStringLiteral("MarkdownImporter::exportToString: Phase 4.");
    VE_LOG_WARNING(outError);
    return {};
}

} // namespace vela
