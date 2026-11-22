// ════════════════════════════════════════════════════════════════════════════
//  DocumentController.h — Central coordinator for document operations
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QObject>
#include <QPointer>
#include <QString>

// QPointer<Document> needs Document to be complete.
#include "document/Document.h"

namespace vela {

class DocxImporter;
class DocxExporter;

/**
 * @brief The DocumentController owns the active Document and routes operations
 *        (open, save, undo/redo, export) to the appropriate handlers.
 *
 * Phase 0: only supports in-memory docs with .docx round-trip via stub
 *          handlers. The wire-up is real; the file IO is stubbed.
 */
class DocumentController : public QObject
{
    Q_OBJECT
public:
    explicit DocumentController(QObject* parent = nullptr);
    ~DocumentController() override;

    /// The currently active document, or nullptr if none.
    Document* activeDocument() const { return m_activeDocument; }

    /// Convenience: returns the active doc's dirty state.
    bool hasUnsavedChanges() const;

public slots:
    bool openDocument(const QString& filePath);
    bool saveDocument(const QString& filePath);
    void closeActiveDocument();
    void newDocument();
    void undo();
    void redo();

signals:
    void activeDocumentChanged();
    void dirtyStateChanged(bool dirty);
    void errorOccurred(const QString& message);

private:
    void setActiveDocument(Document* doc);

    QPointer<Document>     m_activeDocument;
    DocxImporter*          m_importer = nullptr;
    DocxExporter*          m_exporter = nullptr;
};

} // namespace vela
