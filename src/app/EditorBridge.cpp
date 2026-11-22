// ════════════════════════════════════════════════════════════════════════════
//  EditorBridge.cpp — Async bridge (v0.3.1 — CRIT-06)
// ════════════════════════════════════════════════════════════════════════════
#include "app/EditorBridge.h"

#include "document/DocumentController.h"
#include "document/Document.h"
#include "document/TabSession.h"
#include "util/Logger.h"

#include <QDateTime>
#include <QJsonObject>
#include <QRegularExpression>
#include <QWebChannel>

namespace vela {

namespace {
/// Translate "bold", "italic", "heading:h1", "toggleTaskList"… to controller calls.
QVariantMap executeLocal(const QString& cmd, const QVariantMap& args,
                         DocumentController* dc, TabSession* session)
{
    QVariantMap r;
    r["command"] = cmd;

    if (!dc) {
        r["ok"]    = false;
        r["error"] = QStringLiteral("no controller bound");
        return r;
    }

    Document* active = dc->activeDocument();

    if (cmd == "openDocument") {
        r["ok"] = dc->openDocument(args.value("path").toString());
    } else if (cmd == "saveDocument") {
        // No explicit path means "save over the current file"; an untitled
        // document has no path, so the caller must use saveDocumentAs.
        QString path = args.value("path").toString();
        if (path.isEmpty() && active) path = active->filePath();
        if (path.isEmpty()) {
            r["ok"]    = false;
            r["error"] = QStringLiteral("document has no path; use saveDocumentAs");
        } else {
            r["ok"] = dc->saveDocument(path);
        }
    } else if (cmd == "saveDocumentAs") {
        const QString path = args.value("path").toString();
        if (active) active->setFilePath(path);
        r["ok"] = dc->saveDocument(path);
    } else if (cmd == "newDocument") {
        dc->newDocument();
        r["ok"] = true;
    } else if (cmd == "closeDocument") {
        dc->closeActiveDocument();
        r["ok"] = true;
    } else if (cmd == "getBody") {
        if (!active) {
            r["ok"]    = false;
            r["error"] = QStringLiteral("no active document");
        } else {
            r["body"] = active->body().toVariantMap();
            r["ok"]   = true;
        }
    } else if (cmd == "setBody") {
        if (!active) {
            r["ok"]    = false;
            r["error"] = QStringLiteral("no active document");
        } else {
            active->setBody(QJsonObject::fromVariantMap(args.value("body").toMap()));
            r["ok"] = true;
        }
    } else if (cmd == "getPlainText") {
        r["text"] = active ? active->plainText() : QString();
        r["ok"]   = true;
    } else if (cmd == "getHtml" || cmd == "getStats") {
        // Serialised HTML only exists inside the editor; these are answered on
        // the JS side and the bridge call is a heartbeat.
        r["ok"] = true;
    } else if (cmd == "applyFormatCommand") {
        // The JS side already applied the format to its TipTap editor;
        // this slot records the operation for the UndoManager.
        if (session) session->markDirty();
        r["ok"] = true;
    } else if (cmd == "getCurrentFilePath") {
        r["path"] = session ? session->filePath() : QString();
        r["ok"]   = true;
    } else if (cmd == "isDirty") {
        r["dirty"] = session && session->isDirty();
        r["ok"]    = true;
    } else if (cmd == "reportError") {
        // Diagnostic channel used by vela-bridge.js when the page catches
        // something the C++ side should know about.
        const QString where = args.value(QStringLiteral("where"), QStringLiteral("editor")).toString();
        VE_LOG_WARNING("EditorBridge: JS error in " << where << ": "
                        << args.value("message").toString());
        r["ok"] = true;
    } else {
        // Unknown command — surface to C++ but accept (JS may have its own executor).
        VE_LOG_WARNING("EditorBridge: unknown command " << cmd);
        r["ok"]    = true;
        r["noop"]  = true;
    }
    return r;
}
} // namespace

EditorBridge::EditorBridge(DocumentController* controller, QObject* parent)
    : QObject(parent)
    , m_controller(controller)
{
    // Single 100 ms ticker — we use it as a sweeping mechanism for the watchdog.
    m_watchdog.setSingleShot(false);
    m_watchdog.setInterval(100);
    connect(&m_watchdog, &QTimer::timeout, this, [this] {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        QList<QUuid> stale;
        for (auto it = m_pending.begin(); it != m_pending.end(); ++it) {
            if (now - it.value().startedMs > m_watchdogMs)
                stale.push_back(it.key());
        }
        for (const QUuid& id : stale) {
            const Pending p = m_pending.take(id);
            // Resolve the caller's promise rather than leaving it hanging; JS
            // only ever sees commandTimeout as a diagnostic.
            emit commandTimeout(p.requestId, p.command);
            QVariantMap payload;
            payload[QStringLiteral("command")] = p.command;
            payload[QStringLiteral("ok")]      = false;
            payload[QStringLiteral("error")]   = QStringLiteral("timeout after %1 ms").arg(m_watchdogMs);
            emit commandResult(p.requestId, false, payload);
        }
    });
    m_watchdog.start();
}

void EditorBridge::registerOnPage(QObject* webChannel)
{
    auto* ch = qobject_cast<QWebChannel*>(webChannel);
    if (!ch)
        return;
    ch->registerObject(QStringLiteral("vela"), this);
}

qint64 EditorBridge::invokeCommand(const QString& command, const QVariantMap& args)
{
    const qint64 id = m_nextId++;
    const QUuid  u  = QUuid::createUuid();

    Pending p;
    p.command   = command;
    p.requestId = id;
    p.startedMs = QDateTime::currentMSecsSinceEpoch();
    m_pending.insert(u, p);

    // Resolved synchronously on the C++ side and posted back through
    // commandResult so JS can `await` it. The watchdog stays armed as a safety
    // net for commands that a future async path fails to complete.
    const QVariantMap payload = executeLocal(command, args, m_controller, m_session);
    const bool ok = payload.value(QStringLiteral("ok"), true).toBool();
    completeRequest(u, ok, payload);
    return id;
}

void EditorBridge::completeRequest(QUuid id, bool ok, const QVariantMap& payload)
{
    auto it = m_pending.find(id);
    if (it == m_pending.end())
        return;
    // Echo the requestId invokeCommand() handed to JS — not a uuid fragment,
    // which JS has no way to match against.
    const qint64 requestId = it.value().requestId;
    m_pending.erase(it);
    emit commandResult(requestId, ok, payload);
}

void EditorBridge::watchdogFired(QUuid) { /* wired above via lambda */ }

// ─── Legacy slot implementations ───────────────────────────────────────────
bool EditorBridge::openDocument(const QString& path)
{
    if (!m_controller) return false;
    return m_controller->openDocument(path);
}

bool EditorBridge::saveDocument()
{
    if (!m_controller || !m_controller->activeDocument()) return false;
    const QString path = m_controller->activeDocument()->filePath();
    if (path.isEmpty()) return false;      // untitled — caller must use saveDocumentAs
    return m_controller->saveDocument(path);
}

bool EditorBridge::saveDocumentAs(const QString& path)
{
    if (!m_controller) return false;
    if (m_controller->activeDocument())
        m_controller->activeDocument()->setFilePath(path);
    return m_controller->saveDocument(path);
}

void EditorBridge::newDocument()
{
    if (m_controller) m_controller->newDocument();
}

void EditorBridge::closeDocument()
{
    if (m_controller) m_controller->closeActiveDocument();
}

QVariantMap EditorBridge::getBody() const
{
    if (!m_controller || !m_controller->activeDocument()) return {};
    return m_controller->activeDocument()->body().toVariantMap();
}

bool EditorBridge::setBody(const QVariantMap& body)
{
    if (!m_controller || !m_controller->activeDocument()) return false;
    m_controller->activeDocument()->setBody(QJsonObject::fromVariantMap(body));
    return true;
}

QString     EditorBridge::getHtml() const            { return {}; }

QString EditorBridge::getPlainText() const
{
    if (!m_controller || !m_controller->activeDocument()) return {};
    return m_controller->activeDocument()->plainText();
}

void EditorBridge::applyFormatCommand(const QString& command)
{
    if (m_session) m_session->markDirty();
    Q_UNUSED(command);
}

QVariantMap EditorBridge::getStats() const
{
    static const QRegularExpression whitespace(QStringLiteral("\\s+"));

    const QString text = getPlainText();
    const QStringList words = text.split(whitespace, Qt::SkipEmptyParts);

    QString noSpaces = text;
    noSpaces.remove(whitespace);

    QVariantMap stats;
    stats[QStringLiteral("words")]               = words.size();
    stats[QStringLiteral("characters")]          = text.size();
    stats[QStringLiteral("charactersNoSpaces")]  = noSpaces.size();
    return stats;
}

void EditorBridge::reportStats(int words, int characters, int pages)
{
    emit statsChanged(words, characters, pages);
}

QString     EditorBridge::getCurrentFilePath() const
{
    return m_session ? m_session->filePath() : QString();
}

bool EditorBridge::isDirty() const
{
    return m_session && m_session->isDirty();
}

} // namespace vela
