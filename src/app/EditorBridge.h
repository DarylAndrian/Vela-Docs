// ════════════════════════════════════════════════════════════════════════════
//  EditorBridge.h — Async QWebChannel bridge with error handling (v0.3.1 — CRIT-06)
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QUuid>
#include <QVariantMap>

namespace vela {

class DocumentController;
class TabSession;
class MainWindow;

/**
 * @brief The C++ ↔ JavaScript bridge for the embedded TipTap editor.
 *
 * v0.3.1 redesign:
 *   - Every JS-facing call now returns a `qint64 requestId` and the result
 *     comes back through the `commandResult(id, ok, payload)` signal.
 *   - 5-second watchdog; if a result doesn't arrive, the slot fires with
 *     `ok = false` and reason = "timeout".
 *   - JS exceptions propagate via the `errorReported(message)` channel.
 *   - Still keeps the legacy fire-and-forget slots for backward compatibility.
 *
 * Registered on the QWebEnginePage's QWebChannel; accessible to JS as
 * `window.vela`.
 */
class EditorBridge : public QObject
{
    Q_OBJECT
public:
    explicit EditorBridge(DocumentController* controller, QObject* parent = nullptr);

    /// Bind to a TabSession (v0.3.1 multi-tab support).
    void bindSession(TabSession* session) { m_session = session; }

    /// Connect to a QWebEnginePage's QWebChannel.
    Q_INVOKABLE void registerOnPage(QObject* webChannel);

    /// Number of pending requests — used in tests.
    int pendingCount() const { return m_pending.size(); }

    /// Manual timeout override (ms). Tests use a shorter value.
    void setWatchdogMs(int ms) { m_watchdogMs = ms; }

public slots:
    // v0.3.1: async command dispatch.
    // JS calls one of these and gets a requestId; result follows via commandResult signal.
    qint64 invokeCommand(const QString& command, const QVariantMap& args = QVariantMap());

    // Document operations (legacy — synchronous, fire-and-forget).
    bool openDocument(const QString& path);
    bool saveDocument();
    bool saveDocumentAs(const QString& path);
    void newDocument();
    void closeDocument();

    // Body operations
    QVariantMap getBody() const;
    bool setBody(const QVariantMap& body);
    QString getHtml() const;
    QString getPlainText() const;

    // Format commands
    void applyFormatCommand(const QString& command);

    // Stats
    QVariantMap getStats() const;
    /// Pushed by the editor on every change so the native status bar stays live.
    /// JS cannot receive a C++ signal as a return value, hence a dedicated slot.
    void reportStats(int words, int characters, int pages);

    // Identity
    QString getCurrentFilePath() const;
    bool isDirty() const;

signals:
    // Emitted to JS — results of async commands.
    void commandResult(qint64 requestId, bool ok, const QVariantMap& payload);
    /// Watchdog fired and we gave up.
    void commandTimeout(qint64 requestId, const QString& command);
    /// JS reported an exception.
    void errorReported(const QString& where, const QString& message);

    // Legacy — body / document state events.
    void bodyChanged(const QVariantMap& body);
    void documentLoaded(const QString& path);
    void dirtyStateChanged(bool dirty);
    void statsChanged(int words, int characters, int pages);

private slots:
    void watchdogFired(QUuid id);

private:
    /// Map of pending requests, keyed by an internal uuid.
    struct Pending
    {
        QString command;
        qint64  requestId;   // the id invokeCommand() returned to JS
        qint64  startedMs;
    };
    QHash<QUuid, Pending> m_pending;
    QTimer m_watchdog;
    int    m_watchdogMs = 5000;
    qint64 m_nextId = 1;

    void completeRequest(QUuid id, bool ok, const QVariantMap& payload);

    DocumentController* m_controller = nullptr;
    TabSession*         m_session    = nullptr;
};

} // namespace vela
