// ════════════════════════════════════════════════════════════════════════════
//  TabSession.cpp
//
//  The session is a thin owner of one DocumentController plus the identity and
//  dirty/save bookkeeping the tab strip needs. All file IO is delegated to the
//  controller so the .docx round trip lives in exactly one place.
// ════════════════════════════════════════════════════════════════════════════
#include "document/TabSession.h"

#include "document/Document.h"
#include "util/Logger.h"

#include <QFileInfo>

namespace vela {

TabSession::TabSession(QObject* parent)
    : QObject(parent)
    , m_id(QUuid::createUuid())
    , m_controller(std::make_unique<DocumentController>(this))
{
    wireController();
}

TabSession::~TabSession() = default;

std::unique_ptr<TabSession> TabSession::create(const QString& path)
{
    auto session = std::make_unique<TabSession>();
    session->m_filePath = path;

    if (path.isEmpty() || !session->m_controller->openDocument(path)) {
        if (!path.isEmpty())
            VE_LOG_WARNING("TabSession::create could not load " << path
                            << "; starting empty");
        session->m_controller->newDocument();
        session->m_lastSaved = QDateTime::currentDateTime();
    } else {
        session->m_lastSaved = QFileInfo(path).lastModified();
    }

    if (Document* doc = session->document()) {
        doc->setFilePath(session->m_filePath);
        doc->setDirty(false);
    }
    session->m_dirty = false;
    return session;
}

std::unique_ptr<TabSession> TabSession::open(const QString& path)
{
    auto session = std::make_unique<TabSession>();
    if (!session->load(path)) {
        VE_LOG_WARNING("TabSession::open failed for " << path);
        return nullptr;
    }
    return session;
}

bool TabSession::load(const QString& path)
{
    if (!m_controller->openDocument(path)) return false;
    m_filePath = path;
    m_lastSaved = QFileInfo(path).lastModified();
    emit titleChanged(title());
    return true;
}

QString TabSession::title() const
{
    if (!m_filePath.isEmpty())
        return QFileInfo(m_filePath).fileName();
    return QStringLiteral("Untitled");
}

void TabSession::markDirty()
{
    if (m_dirty) return;
    m_dirty = true;
    emit dirtyChanged(true);
}

bool TabSession::save()
{
    // An untitled tab has nowhere to write; the caller must use saveAs().
    if (m_filePath.isEmpty())
        return false;

    if (!m_controller->saveDocument(m_filePath))
        return false;

    m_lastSaved = QDateTime::currentDateTime();
    emit saveStateChanged(m_lastSaved);
    return true;
}

bool TabSession::saveAs(const QString& path)
{
    if (!m_controller->saveDocument(path)) return false;
    const bool pathChanged = (m_filePath != path);
    m_filePath = path;
    if (pathChanged)
        emit titleChanged(title());
    m_lastSaved = QDateTime::currentDateTime();
    emit saveStateChanged(m_lastSaved);
    return true;
}

void TabSession::wireController()
{
    // Document::dirtyChanged -> DocumentController::dirtyStateChanged -> here.
    // The controller already clears the flag on a successful save, so this is
    // the single source of truth for the session's dirty state.
    connect(m_controller.get(), &DocumentController::dirtyStateChanged,
            this, [this](bool dirty) {
        if (dirty) {
            markDirty();
        } else if (m_dirty) {
            m_dirty = false;
            emit dirtyChanged(false);
        }
    });
}

} // namespace vela
