// ════════════════════════════════════════════════════════════════════════════
//  DocxImporter.h — Read OOXML (.docx) files into Document model
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>

namespace vela {

class Document;

/**
 * @brief Reads a .docx file (Office Open XML) and populates a Document.
 *
 * Phase 0: stub. Phase 1: full implementation.
 * .docx is a ZIP containing XML parts: document.xml, styles.xml,
 * numbering.xml, header*.xml, footer*.xml, footnotes.xml, media/*, etc.
 */
class DocxImporter : public QObject
{
    Q_OBJECT
public:
    explicit DocxImporter(QObject* parent = nullptr);

    /// Parse a .docx file and return a populated JSON body.
    /// Returns false on error.
    bool importFromFile(const QString& path, QJsonObject& outBody, QString& outError);

signals:
    void progressChanged(qreal fraction);
};

} // namespace vela
