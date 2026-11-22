// MacroRecorder.h — Macro recorder + sandbox (v0.3.5 — POLISH-06)
#pragma once
#include <QObject>
#include <QString>
#include <QStringList>

namespace vela {

class EditorBridge;

/**
 * @brief Captures TipTap commands into a JSON timeline; replays them.
 *
 * Live recording taps every command through `EditorBridge::invokeCommand`
 * and serialises each into a script. Replay re-enters the same set of
 * commands through the bridge. Macros can also be hand-authored JavaScript
 * that calls `vela.*` APIs; the `MacrosEditor` runs them in a QJSEngine
 * sandbox.
 */
class MacroRecorder : public QObject
{
    Q_OBJECT
public:
    explicit MacroRecorder(EditorBridge* bridge, QObject* parent = nullptr);

    bool   isRecording() const { return m_active; }
    void   start();
    void   stop();

    /// Save the recorded timeline as a JavaScript file (or as JSON if `asJson`).
    QString exportScript(bool asJson = false) const;

    /// Load and execute a macro (either JS or JSON timeline).
    void   run(const QString& code, bool isJson = false);

signals:
    void recordedCommand(const QString& command, const QVariantMap& args);
    void replayError(const QString& message);

private:
    struct Entry { QString command; QVariantMap args; };
    QVector<Entry> m_timeline;
    bool           m_active = false;
    EditorBridge*  m_bridge = nullptr;
};

} // namespace vela
