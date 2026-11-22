// ════════════════════════════════════════════════════════════════════════════
//  StylesPane.h — Sidebar styles panel (v0.3.2 — MAJOR-04)
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QListWidget>
#include <memory>

namespace vela {

class Document;
class StyleSheet;

/**
 * @brief Sidebar widget that lists every style and applies it on click.
 *
 * Right-click on a custom (non-built-in) style offers "Update Style to
 * Match Selection" and "Delete". The pane subscribes to the document's
 * selection so the highlight tracks the cursor.
 */
class StylesPane : public QListWidget
{
    Q_OBJECT
public:
    explicit StylesPane(QWidget* parent = nullptr);
    ~StylesPane() override;

    /// Re-bind to a new document. The pane calls `refresh()` automatically.
    void bindDocument(Document* doc);

public slots:
    /// Re-read the styles list and rebuild the rows.
    void refresh();

signals:
    void styleActivated(const QString& aliasId);

private slots:
    void onItemActivated(QListWidgetItem* item);

private:
    Document*        m_doc = nullptr;
    std::unique_ptr<StyleSheet> m_styles;
};

} // namespace vela
