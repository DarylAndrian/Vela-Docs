// ════════════════════════════════════════════════════════════════════════════
//  TabSession.h — Per-tab document state (v0.3.1 — CRIT-02)
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QUuid>

#include "document/DocumentController.h"   // inline document() dereferences it

#include <memory>

namespace vela {

class Document;

/**
 * @brief Per-tab document session.
 *
 * Each open tab owns one TabSession. The session owns its Document + Controller
 * lifecycle and tracks undo/redo depth, dirty state, and the last-saved
 * timestamp used by the AutoSaver.
 *
 * Lives in `src/document/` (not `app/`) because the session is the unit of
 * document state, not UI state — the TabBar in `src/app/` only displays these.
 */
class TabSession : public QObject
{
    Q_OBJECT
public:
    explicit TabSession(QObject* parent = nullptr);
    ~TabSession() override;

    /// New empty document, optional starting file path.
    static std::unique_ptr<TabSession> create(const QString& path = QString());
    /// Load from a file on disk; returns nullptr on failure.
    static std::unique_ptr<TabSession> open(const QString& path);
    bool load(const QString& path);

    /// Identity.
    QUuid id() const { return m_id; }

    /// Display title — file name, or "Untitled" for a new document.
    QString title() const;
    /// Full path, or empty for unsaved.
    QString filePath() const { return m_filePath; }

    /// Underlying document state. The DocumentController owns the Document
    /// (it parents it as a QObject), so the session hands out a raw pointer.
    Document*           document()       { return m_controller ? m_controller->activeDocument() : nullptr; }
    const Document*     document() const { return m_controller ? m_controller->activeDocument() : nullptr; }
    DocumentController* controller()     { return m_controller.get(); }

    /// Dirty state — modified since last save or open.
    bool   isDirty() const { return m_dirty; }
    /// Last time the file was written (or opened).
    QDateTime lastSaved() const { return m_lastSaved; }

    /// Mark as dirty (called by controller when content changes).
    void markDirty();
    /// Persist to current filePath() or fall back to autosave.
    bool save();
    /// Persist to a new path; updates filePath().
    bool saveAs(const QString& path);

signals:
    void titleChanged(const QString& newTitle);
    void dirtyChanged(bool dirty);
    void saveStateChanged(const QDateTime& lastSaved);

private:
    void wireController();

    QUuid     m_id;
    QString   m_filePath;
    /// Owns the Document (as its QObject parent) and the importer/exporter.
    std::unique_ptr<DocumentController> m_controller;
    bool      m_dirty = false;
    QDateTime m_lastSaved;
};

} // namespace vela
