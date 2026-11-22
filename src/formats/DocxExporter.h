// ════════════════════════════════════════════════════════════════════════════
//  DocxExporter.h — Write Document model to OOXML (.docx)
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>

namespace vela {

class Document;

/**
 * @brief Serializes a Document to .docx format.
 *
 * Phase 0: stub. Phase 1: full implementation.
 */
class DocxExporter : public QObject
{
    Q_OBJECT
public:
    explicit DocxExporter(QObject* parent = nullptr);

    /// Write a document to a .docx file. Returns false on error.
    bool exportToFile(const QString& path, const QJsonObject& body, QString& outError);

signals:
    void progressChanged(qreal fraction);
};

} // namespace vela
