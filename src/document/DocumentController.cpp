// ════════════════════════════════════════════════════════════════════════════
//  DocumentController.cpp — Phase 1 (real OOXML round-trip)
// ════════════════════════════════════════════════════════════════════════════
#include "document/DocumentController.h"
#include "document/Document.h"
#include "formats/DocxImporter.h"
#include "formats/DocxExporter.h"
#include "util/Logger.h"

#include <QFileInfo>

namespace vela {

DocumentController::DocumentController(QObject* parent)
    : QObject(parent)
    , m_importer(new DocxImporter(this))
    , m_exporter(new DocxExporter(this))
{
}

DocumentController::~DocumentController() = default;

bool DocumentController::hasUnsavedChanges() const
{
    return m_activeDocument && m_activeDocument->isDirty();
}

bool DocumentController::openDocument(const QString& filePath)
{
    VE_LOG_INFO("DocumentController::openDocument: " << filePath);

    const QFileInfo info(filePath);
    if (!info.exists() || !info.isReadable()) {
        emit errorOccurred(tr("File not found or not readable: %1").arg(filePath));
        return false;
    }

    const QString suffix = info.suffix().toLower();

    if (suffix == QLatin1String("docx")) {
        // Phase 1: real OOXML parsing
        QJsonObject body;
        QString err;
        if (!m_importer->importFromFile(filePath, body, err)) {
            emit errorOccurred(tr("Failed to open .docx: %1").arg(err));
            return false;
        }
        auto* doc = new Document(this);
        doc->setFilePath(filePath);
        doc->setBody(body);
        doc->setDirty(false);
        connect(doc, &Document::dirtyChanged,
                this, &DocumentController::dirtyStateChanged);
        connect(doc, &Document::bodyChanged, this, [doc]() {
            doc->setDirty(true);
        });
        setActiveDocument(doc);
        return true;
    }

    emit errorOccurred(tr("Choose a Word document with the .docx file extension."));
    return false;
}

bool DocumentController::saveDocument(const QString& filePath)
{
    VE_LOG_INFO("DocumentController::saveDocument: " << filePath);
    if (!m_activeDocument) {
        emit errorOccurred(tr("No document to save."));
        return false;
    }

    const QFileInfo info(filePath);
    const QString suffix = info.suffix().toLower();

    if (suffix == QLatin1String("docx")) {
        QString err;
        if (!m_exporter->exportToFile(filePath, m_activeDocument->body(), err)) {
            emit errorOccurred(tr("Failed to save .docx: %1").arg(err));
            return false;
        }
        m_activeDocument->setFilePath(filePath);
        m_activeDocument->setDirty(false);
        return true;
    }

    emit errorOccurred(tr("Choose the .docx file extension to save this document."));
    return false;
}

void DocumentController::closeActiveDocument()
{
    setActiveDocument(nullptr);
}

void DocumentController::newDocument()
{
    restoreDraft({});
}

void DocumentController::restoreDraft(const QJsonObject& body)
{
    auto* doc = new Document(this);
    if (!body.isEmpty()) {
        doc->setBody(body);
        doc->setDirty(true);
    }
    connect(doc, &Document::dirtyChanged,
            this, &DocumentController::dirtyStateChanged);
    connect(doc, &Document::bodyChanged, this, [doc]() {
        doc->setDirty(true);
    });
    setActiveDocument(doc);
}

void DocumentController::undo()
{
    VE_LOG_DEBUG("undo() — Phase 1 will hook up ProseMirror history via QWebChannel");
}

void DocumentController::redo()
{
    VE_LOG_DEBUG("redo() — Phase 1 will hook up ProseMirror history via QWebChannel");
}

void DocumentController::setActiveDocument(Document* doc)
{
    if (m_activeDocument == doc) return;
    if (m_activeDocument) {
        disconnect(m_activeDocument, nullptr, this, nullptr);
        m_activeDocument->deleteLater();
    }
    m_activeDocument = doc;
    emit activeDocumentChanged();
    emit dirtyStateChanged(doc && doc->isDirty());
}

} // namespace vela
