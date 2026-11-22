// ════════════════════════════════════════════════════════════════════════════
//  PageSetupDialog.h — v0.5.0
//
//  Native Qt dialog for page setup (paper size, orientation, margins). Mirrors
//  the visual design in stitch_vela_docs_ui_system/page_setup_dialog/.
//
//  On accept, the dialog pushes a JSON settings blob into the web editor via
//  window.__velaSetPageSettings(json) (registered by page-settings.js), which
//  applies the geometry to the PaginationEngine and the C++ PageStyle model
//  stays in sync via window.vela.setPageSettings.
//
//  The settings JSON shape matches page-settings.js DEFAULT_SETTINGS:
//    { mode, size, orientation, marginTop/Right/Bottom/Left (inches),
//      pageGap, headerText, footerText, differentFirstPage,
//      pageNumberFormat, pageNumberStartAt }
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QDialog>
#include <QJsonObject>

class QComboBox;
class QRadioButton;
class QDoubleSpinBox;
class QWebEngineView;

namespace vela {

class PageSetupDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * @param view   The embedded editor view (for read-back / push-settings).
     *               May be nullptr in the headless unit test.
     * @param parent Owning widget.
     */
    explicit PageSetupDialog(QWebEngineView* view = nullptr, QWidget* parent = nullptr);

    /** Settings as JSON (matches page-settings.js DEFAULT_SETTINGS shape). */
    QJsonObject settingsJson() const;
    void setSettings(const QJsonObject& s);

private slots:
    void applyAndClose();
    void applyOnly();

private:
    void buildUi();
    void pushToEditor();

    QWebEngineView* m_view;
    QComboBox*      m_paperSize  = nullptr;
    QRadioButton*   m_portrait   = nullptr;
    QRadioButton*   m_landscape  = nullptr;
    QDoubleSpinBox* m_marginTop    = nullptr;
    QDoubleSpinBox* m_marginBottom = nullptr;
    QDoubleSpinBox* m_marginLeft   = nullptr;
    QDoubleSpinBox* m_marginRight  = nullptr;
};

} // namespace vela
