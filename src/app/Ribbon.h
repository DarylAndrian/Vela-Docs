// ════════════════════════════════════════════════════════════════════════════
//  Ribbon.h — Compact formatting toolbar (Stitch "Vela Precision" design)
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QToolBar>

class QComboBox;

namespace vela {

/**
 * @brief A compact formatting toolbar that sits below the document tabs.
 *
 * From the Stitch design: a single horizontal row with font/size pickers,
 * B/I/U, color pickers, alignment, lists, headings. No Word-style tabs —
 * the user picks from the menu bar instead.
 *
 * All actions emit `formatCommand(QString)` so the host (MainWindow) can
 * forward them to the editor (QWebChannel → TipTap).
 */
class Ribbon : public QToolBar
{
    Q_OBJECT
public:
    explicit Ribbon(QWidget* parent = nullptr);

    /// Emit formatCommand(). Public because the button factory below is a file
    /// local helper, not a member, so it cannot reach a private slot.
    void emitCommand(const QString& cmd);

signals:
    /// Emitted with a command identifier when the user interacts with the
    /// toolbar. Phase 1+ will forward these to the TipTap editor.
    void formatCommand(const QString& command);

private:
    QComboBox* m_fontFamily = nullptr;
    QComboBox* m_fontSize   = nullptr;
};

} // namespace vela