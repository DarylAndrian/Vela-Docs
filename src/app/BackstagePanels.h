// ════════════════════════════════════════════════════════════════════════════
//  BackstagePanels.h — v0.4.0
//
//  Eight concrete BackstagePanel subclasses that fill the right-side
//  content area of the Microsoft Word-style Files tab. Each panel emits
//  signals that MainWindow connects to its existing service layer.
//
//  - SavePanel       — Save the current document
//  - SaveAsPanel     — Save As another format
//  - OpenPanel       — Open an existing file
//  - PrintPanel      — Printer / copies / page range + Print
//  - PrintPreviewPanel — QPrintPreviewWidget embedded
//  - OptionsPanel    — Hosts PreferencesDialog (or a stub button)
//  - HelpPanel       — About + user guide + keyboard cheat-sheet
//  - RecentPanel     — Grid of recent files
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include "app/BackstagePanel.h"

#include <QPointer>
#include <QString>

class QLabel;
class QPushButton;
class QComboBox;
class QSpinBox;
class QGridLayout;
class QVBoxLayout;

namespace vela {

class DocumentController;
class RecentFiles;

#ifdef VE_HAS_WEBENGINE
class QWebEngineView;
class QPrinter;
class QPrintPreviewWidget;
#endif

// ── Save ──────────────────────────────────────────────────────────────
class SavePanel : public BackstagePanel
{
    Q_OBJECT
public:
    explicit SavePanel(DocumentController* controller, RecentFiles* recent,
                       QWidget* parent = nullptr);
    QString panelId() const override { return QStringLiteral("save"); }
    QString panelTitle() const override { return tr("Save"); }

signals:
    void saveRequested();

private:
    QPointer<DocumentController> m_controller;
};

// ── Save As ──────────────────────────────────────────────────────────
class SaveAsPanel : public BackstagePanel
{
    Q_OBJECT
public:
    explicit SaveAsPanel(DocumentController* controller, RecentFiles* recent,
                         QWidget* parent = nullptr);
    QString panelId() const override { return QStringLiteral("saveAs"); }
    QString panelTitle() const override { return tr("Save As"); }

signals:
    /// Emitted when the user clicks one of the format buttons.
    /// `format` is one of: docx, odt, rtf, pdf, md, html, txt.
    void saveAsRequested(const QString& format);

private:
    QPointer<DocumentController> m_controller;
};

// ── Open ─────────────────────────────────────────────────────────────
class OpenPanel : public BackstagePanel
{
    Q_OBJECT
public:
    explicit OpenPanel(DocumentController* controller, RecentFiles* recent,
                       QWidget* parent = nullptr);
    QString panelId() const override { return QStringLiteral("open"); }
    QString panelTitle() const override { return tr("Open"); }

signals:
    void openRequested();

private:
    QPointer<DocumentController> m_controller;
    QPointer<RecentFiles>         m_recent;
};

// ── Print ────────────────────────────────────────────────────────────
class PrintPanel : public BackstagePanel
{
    Q_OBJECT
public:
    explicit PrintPanel(QWidget* parent = nullptr);
    QString panelId() const override { return QStringLiteral("print"); }
    QString panelTitle() const override { return tr("Print"); }

signals:
    void printRequested(const QString& printerName, int copies,
                        const QString& pageRange);

private:
    QComboBox* m_printerCombo  = nullptr;
    QSpinBox*  m_copies       = nullptr;
    QComboBox* m_pages        = nullptr;
    QComboBox* m_pageRangeEdit = nullptr;
};

// ── Print Preview ────────────────────────────────────────────────────
#ifdef VE_HAS_WEBENGINE
class PrintPreviewPanel : public BackstagePanel
{
    Q_OBJECT
public:
    explicit PrintPreviewPanel(QWebEngineView* webView, QWidget* parent = nullptr);
    QString panelId() const override { return QStringLiteral("printPreview"); }
    QString panelTitle() const override { return tr("Print Preview"); }

    void onShown() override;

signals:
    void previewRequested();

private:
    QPointer<QWebEngineView> m_webView;
    QPointer<QPrintPreviewWidget> m_previewWidget;
};
#endif

// ── Options ──────────────────────────────────────────────────────────
class OptionsPanel : public BackstagePanel
{
    Q_OBJECT
public:
    explicit OptionsPanel(QWidget* parent = nullptr);
    QString panelId() const override { return QStringLiteral("options"); }
    QString panelTitle() const override { return tr("Options"); }

signals:
    void preferencesRequested();

private slots:
    void onPreferencesClicked();
};

// ── Help ─────────────────────────────────────────────────────────────
class HelpPanel : public BackstagePanel
{
    Q_OBJECT
public:
    explicit HelpPanel(QWidget* parent = nullptr);
    QString panelId() const override { return QStringLiteral("help"); }
    QString panelTitle() const override { return tr("Help"); }

signals:
    void aboutRequested();

private slots:
    void onAboutClicked();
};

// ── Recent ───────────────────────────────────────────────────────────
class RecentPanel : public BackstagePanel
{
    Q_OBJECT
public:
    explicit RecentPanel(RecentFiles* recent, QWidget* parent = nullptr);
    QString panelId() const override { return QStringLiteral("recent"); }
    QString panelTitle() const override { return tr("Recent"); }

    void onShown() override;

signals:
    void fileChosen(const QString& filePath);

private slots:
    void onCardClicked();

private:
    QPointer<RecentFiles> m_recent;
};

} // namespace vela