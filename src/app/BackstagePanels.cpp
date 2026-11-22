// ════════════════════════════════════════════════════════════════════════════
//  BackstagePanels.cpp — v0.4.0
// ════════════════════════════════════════════════════════════════════════════
#include "app/BackstagePanels.h"
#include "app/RecentFiles.h"
#include "document/DocumentController.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QtGlobal>

#ifdef VE_HAS_WEBENGINE
#  include <QWebEngineView>
#  include <QPrinter>
#  include <QPrintPreviewWidget>
#endif

namespace vela {

namespace {
    /// Build a common "h2 + summary" header for backstage panels
    QLabel* panelHeader(const QString& title, QWidget* parent) {
        auto* h = new QLabel(title, parent);
        h->setStyleSheet(QStringLiteral(
            "font-size: 26px; font-weight: 300; color: #222; margin: 0 0 8px;"));
        return h;
    }
    QLabel* panelSummary(const QString& text, QWidget* parent) {
        auto* p = new QLabel(text, parent);
        p->setWordWrap(true);
        p->setStyleSheet(QStringLiteral("color: #777587; margin: 0 0 16px;"));
        return p;
    }
    QPushButton* makeButton(const QString& label, const QString& iconText,
                            const QString& objectName, QWidget* parent) {
        auto* b = new QPushButton(iconText + QStringLiteral("  ") + label, parent);
        b->setObjectName(objectName);
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(36);
        b->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background: #FFFFFF;"
            "  border: 1px solid #D8D8E0;"
            "  border-radius: 4px;"
            "  padding: 6px 14px;"
            "  color: #222;"
            "  text-align: left;"
            "}"
            "QPushButton:hover { background: #E1E8FD; color: #3525CD; border-color: #3525CD; }"));
        return b;
    }
    QPushButton* makePrimaryButton(const QString& label, const QString& iconText,
                                   const QString& objectName, QWidget* parent) {
        auto* b = makeButton(label, iconText, objectName, parent);
        b->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background: #3525CD;"
            "  color: white;"
            "  border: 1px solid #3525CD;"
            "  border-radius: 4px;"
            "  padding: 6px 14px;"
            "  font-weight: 600;"
            "  text-align: left;"
            "}"
            "QPushButton:hover { background: #2A1FB0; }"));
        return b;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  SavePanel
// ═══════════════════════════════════════════════════════════════════════════
SavePanel::SavePanel(DocumentController* controller, RecentFiles* /*recent*/,
                     QWidget* parent)
    : BackstagePanel(parent), m_controller(controller)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    layout->addWidget(panelHeader(tr("Save"), this));
    layout->addWidget(panelSummary(tr("Your document is up to date."), this));

    auto* saveBtn = makePrimaryButton(tr("Save"), QStringLiteral("\xE2\x9C\x93"),   // ✓
                                     QStringLiteral("VelaBsSavePrimary"), this);
    connect(saveBtn, &QPushButton::clicked, this, &SavePanel::saveRequested);
    layout->addWidget(saveBtn);

    layout->addSpacing(16);
    auto* altLabel = new QLabel(tr("Save as another format"), this);
    altLabel->setStyleSheet(QStringLiteral("font-weight: 600; color: #464555;"));
    layout->addWidget(altLabel);

    auto* grid = new QGridLayout();
    grid->setSpacing(8);

    struct FormatButton { const char* label; const char* fmt; };
    const FormatButton formats[] = {
        {"Word Document (.docx)", "docx"},
        {"PDF (.pdf)",            "pdf"},
        {"Markdown (.md)",        "md"},
        {"HTML (.html)",          "html"},
        {"OpenDocument (.odt)",   "odt"},
        {"Rich Text (.rtf)",      "rtf"},
        {"Plain Text (.txt)",     "txt"},
    };
    int row = 0, col = 0;
    for (const auto& f : formats) {
        auto* b = makeButton(QString::fromUtf8(f.label),
                             QStringLiteral("\xE2\x9C\x89"),
                             QStringLiteral("VelaBsFormat"), this);
        b->setProperty("velaFmt", QString::fromUtf8(f.fmt));
        connect(b, &QPushButton::clicked, this, [this, b]() {
            emit saveAsRequested(b->property("velaFmt").toString());
        });
        grid->addWidget(b, row, col);
        if (++col >= 2) { col = 0; row++; }
    }
    layout->addLayout(grid);
    layout->addStretch(1);
}

// ═══════════════════════════════════════════════════════════════════════════
//  SaveAsPanel
// ═══════════════════════════════════════════════════════════════════════════
SaveAsPanel::SaveAsPanel(DocumentController* controller, RecentFiles* /*recent*/,
                         QWidget* parent)
    : BackstagePanel(parent), m_controller(controller)
{
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(12);

    layout->addWidget(panelHeader(tr("Save As"), this));
    layout->addWidget(panelSummary(tr("Choose a location and file format."), this));

    struct FormatButton { const char* label; const char* fmt; };
    const FormatButton formats[] = {
        {"Word Document (.docx)", "docx"},
        {"PDF (.pdf)",            "pdf"},
        {"OpenDocument (.odt)",   "odt"},
        {"Rich Text (.rtf)",      "rtf"},
        {"Markdown (.md)",        "md"},
        {"HTML (.html)",          "html"},
        {"Plain Text (.txt)",     "txt"},
    };

    auto* grid = new QGridLayout();
    grid->setSpacing(8);
    int row = 0, col = 0;
    for (const auto& f : formats) {
        auto* b = makeButton(QString::fromUtf8(f.label),
                             QStringLiteral("\xE2\x9C\x89"),
                             QStringLiteral("VelaBsFormat"), this);
        b->setProperty("velaFmt", QString::fromUtf8(f.fmt));
        connect(b, &QPushButton::clicked, this, [this, b]() {
            emit saveAsRequested(b->property("velaFmt").toString());
        });
        grid->addWidget(b, row, col);
        if (++col >= 2) { col = 0; row++; }
    }
    layout->addLayout(grid);
    layout->addStretch(1);
}

// ═══════════════════════════════════════════════════════════════════════════
//  OpenPanel
// ═══════════════════════════════════════════════════════════════════════════
OpenPanel::OpenPanel(DocumentController* controller, RecentFiles* recent,
                     QWidget* parent)
    : BackstagePanel(parent), m_controller(controller), m_recent(recent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(12);

    layout->addWidget(panelHeader(tr("Open"), this));

    auto* browseBtn = makePrimaryButton(tr("Browse…"), QStringLiteral("\xF0\x9F\x93\xA2"),
                                        QStringLiteral("VelaBsBrowse"), this);
    connect(browseBtn, &QPushButton::clicked, this, &OpenPanel::openRequested);
    layout->addWidget(browseBtn);

    layout->addSpacing(8);
    auto* recentLabel = new QLabel(tr("Recent documents"), this);
    recentLabel->setStyleSheet(QStringLiteral("font-weight: 600; color: #464555;"));
    layout->addWidget(recentLabel);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    layout->addWidget(scroll, 1);

    auto* recentGrid = new QWidget(scroll);
    recentGrid->setObjectName(QStringLiteral("VelaBsRecentGrid"));
    auto* gridLayout = new QGridLayout(recentGrid);
    gridLayout->setSpacing(8);
    int row = 0, col = 0;
    if (m_recent) {
        const auto files = m_recent->list();
        if (files.isEmpty()) {
            auto* empty = new QLabel(tr("No recent files."), recentGrid);
            empty->setStyleSheet(QStringLiteral("color: #777587; font-style: italic;"));
            gridLayout->addWidget(empty, 0, 0);
        } else {
            for (const auto& path : files) {
                auto* card = new QPushButton(recentGrid);
                card->setObjectName(QStringLiteral("VelaBsRecentCard"));
                card->setCursor(Qt::PointingHandCursor);
                card->setFixedSize(120, 100);
                card->setStyleSheet(QStringLiteral(
                    "QPushButton#VelaBsRecentCard {"
                    "  background: #FFFFFF;"
                    "  border: 1px solid #D8D8E0;"
                    "  border-radius: 4px;"
                    "  text-align: left;"
                    "  padding: 8px;"
                    "  color: #222;"
                    "}"
                    "QPushButton#VelaBsRecentCard:hover {"
                    "  border-color: #3525CD;"
                    "  background: #F5F5FA;"
                    "}"));
                auto* cardLayout = new QVBoxLayout(card);
                auto* thumb = new QLabel(QStringLiteral("\xF0\x9F\x93\x9D"), card);
                thumb->setStyleSheet(QStringLiteral("font-size: 28px; color: #777587;"));
                cardLayout->addWidget(thumb);
                auto* name = new QLabel(path.section(QLatin1Char('/'), -1), card);
                name->setStyleSheet(QStringLiteral("font-size: 11px; color: #464555;"));
                name->setWordWrap(true);
                cardLayout->addWidget(name);
                card->setProperty("velaPath", path);
                connect(card, &QPushButton::clicked, this, [this, card]() {
                    emit fileChosen(card->property("velaPath").toString());
                });
                gridLayout->addWidget(card, row, col);
                if (++col >= 4) { col = 0; row++; }
            }
        }
    }
    scroll->setWidget(recentGrid);
    layout->addStretch(0);
}

// ═══════════════════════════════════════════════════════════════════════════
//  PrintPanel
// ═══════════════════════════════════════════════════════════════════════════
PrintPanel::PrintPanel(QWidget* parent) : BackstagePanel(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(12);

    layout->addWidget(panelHeader(tr("Print"), this));
    layout->addWidget(panelSummary(
        tr("Choose a printer, the number of copies, and the page range."), this));

    auto* grid = new QGridLayout();
    grid->setSpacing(12);
    int row = 0;

    auto addRow = [&](const QString& label, QWidget* field) {
        auto* lbl = new QLabel(label, this);
        lbl->setStyleSheet(QStringLiteral("color: #464555; font-size: 12px;"));
        grid->addWidget(lbl, row, 0);
        grid->addWidget(field, row, 1);
        row++;
    };

    m_printerCombo = new QComboBox(this);
    m_printerCombo->addItem(tr("Default printer"));
    addRow(tr("Printer"), m_printerCombo);

    m_copies = new QSpinBox(this);
    m_copies->setRange(1, 999);
    m_copies->setValue(1);
    addRow(tr("Copies"), m_copies);

    m_pages = new QComboBox(this);
    m_pages->addItem(tr("All"), QStringLiteral("all"));
    m_pages->addItem(tr("Current page"), QStringLiteral("current"));
    m_pages->addItem(tr("Range…"), QStringLiteral("range"));
    addRow(tr("Pages"), m_pages);

    layout->addLayout(grid);

    auto* printBtn = makePrimaryButton(tr("Print"), QStringLiteral("\xE2\x80\xBA"),
                                       QStringLiteral("VelaBsPrint"), this);
    connect(printBtn, &QPushButton::clicked, this, [this]() {
        emit printRequested(m_printerCombo->currentText(),
                            m_copies->value(),
                            m_pages->currentData().toString());
    });
    layout->addWidget(printBtn);
    layout->addStretch(1);
}

// ═══════════════════════════════════════════════════════════════════════════
//  PrintPreviewPanel
// ═══════════════════════════════════════════════════════════════════════════
#ifdef VE_HAS_WEBENGINE
PrintPreviewPanel::PrintPreviewPanel(QWebEngineView* webView, QWidget* parent)
    : BackstagePanel(parent), m_webView(webView)
{
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(8);

    auto* controls = new QHBoxLayout();
    auto* zoomOut = new QPushButton(QStringLiteral("-"), this);
    auto* zoomIn  = new QPushButton(QStringLiteral("+"), this);
    auto* printBtn = makePrimaryButton(tr("Print"), QStringLiteral("\xE2\x80\xBA"),
                                        QStringLiteral("VelaBsPrintPreview"), this);
    controls->addWidget(zoomOut);
    controls->addWidget(zoomIn);
    controls->addStretch(1);
    controls->addWidget(printBtn);
    layout->addLayout(controls);

    m_previewWidget = new QPrintPreviewWidget(this);
    layout->addWidget(m_previewWidget, 1);

    connect(zoomIn,  &QPushButton::clicked, m_previewWidget, &QPrintPreviewWidget::zoomIn);
    connect(zoomOut, &QPushButton::clicked, m_previewWidget, &QPrintPreviewWidget::zoomOut);
    connect(printBtn, &QPushButton::clicked, this, &PrintPreviewPanel::previewRequested);
}

void PrintPreviewPanel::onShown()
{
    if (!m_previewWidget || !m_webView) return;
    QPrinter p;
    m_previewWidget->setPrinter(p);
    m_previewWidget->updatePreview();
}
#endif

// ═══════════════════════════════════════════════════════════════════════════
//  OptionsPanel
// ═══════════════════════════════════════════════════════════════════════════
OptionsPanel::OptionsPanel(QWidget* parent) : BackstagePanel(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(12);

    layout->addWidget(panelHeader(tr("Options"), this));
    layout->addWidget(panelSummary(
        tr("Vela Docs preferences — Documents, Display, Editing, Privacy, Keybindings, Language, Templates, Mail Merge."),
        this));

    auto* prefsBtn = makePrimaryButton(
        tr("Open Preferences…"), QStringLiteral("\xE2\x9A\x99",
        QStringLiteral("VelaBsPreferences"), this));
    connect(prefsBtn, &QPushButton::clicked, this, &OptionsPanel::onPreferencesClicked);
    layout->addWidget(prefsBtn);
    layout->addStretch(1);
}

void OptionsPanel::onPreferencesClicked()
{
    emit preferencesRequested();
}

// ═══════════════════════════════════════════════════════════════════════════
//  HelpPanel
// ═══════════════════════════════════════════════════════════════════════════
HelpPanel::HelpPanel(QWidget* parent) : BackstagePanel(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(12);

    layout->addWidget(panelHeader(tr("Help"), this));

    auto* grid = new QGridLayout();
    grid->setSpacing(8);
    int row = 0, col = 0;

    auto addBtn = [&](const QString& label, const QString& icon) {
        auto* b = makeButton(label, icon, QStringLiteral("VelaBsHelp"), this);
        grid->addWidget(b, row, col);
        if (++col >= 2) { col = 0; row++; }
        return b;
    };
    connect(addBtn(tr("About Vela Docs"),      QStringLiteral("\xE2\x84\xB9"),
                  &HelpPanel::aboutClicked),  &HelpPanel::aboutClicked);
    addBtn(tr("User guide"),                 QStringLiteral("\xF0\x9F\x93\x96"),
          &HelpPanel::aboutClicked);
    addBtn(tr("Changelog"),                  QStringLiteral("\xF0\x9F\x97\x93"),
          &HelpPanel::aboutClicked);
    addBtn(tr("Keyboard shortcuts"),         QStringLiteral("\xE2\x9C\xA8"),
          &HelpPanel::aboutClicked);
    layout->addLayout(grid);

    auto* cheatLabel = new QLabel(tr("Common shortcuts"), this);
    cheatLabel->setStyleSheet(QStringLiteral("font-weight: 600; color: #464555; margin-top: 16px;"));
    layout->addWidget(cheatLabel);

    auto* shortcuts = new QLabel(this);
    shortcuts->setText(QStringLiteral(
        "<table style='border-collapse:collapse'>"
        "<tr><td><b>Bold</b></td><td><tt>Ctrl+B</tt></td></tr>"
        "<tr><td><b>Italic</b></td><td><tt>Ctrl+I</tt></td></tr>"
        "<tr><td><b>Underline</b></td><td><tt>Ctrl+U</tt></td></tr>"
        "<tr><td><b>Save</b></td><td><tt>Ctrl+S</tt></td></tr>"
        "<tr><td><b>Files tab</b></td><td><tt>Alt+F</tt></td></tr>"
        "<tr><td><b>Heading 1</b></td><td><tt>Ctrl+1</tt></td></tr>"
        "<tr><td><b>Bullet list</b></td><td><tt>Ctrl+Shift+L</tt></td></tr>"
        "<tr><td><b>Insert link</b></td><td><tt>Ctrl+K</tt></td></tr>"
        "<tr><td><b>Find</b></td><td><tt>Ctrl+F</tt></td></tr>"
        "<tr><td><b>Print</b></td><td><tt>Ctrl+P</tt></td></tr>"
        "</table>"));
    shortcuts->setTextFormat(Qt::RichText);
    shortcuts->setStyleSheet(QStringLiteral("font-size: 12px;"));
    layout->addWidget(shortcuts);
    layout->addStretch(1);
}

void HelpPanel::onAboutClicked()
{
    emit aboutRequested();
}

// ═══════════════════════════════════════════════════════════════════════════
//  RecentPanel
// ═══════════════════════════════════════════════════════════════════════════
RecentPanel::RecentPanel(RecentFiles* recent, QWidget* parent)
    : BackstagePanel(parent), m_recent(recent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(12);

    layout->addWidget(panelHeader(tr("Recent"), this));

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    layout->addWidget(scroll, 1);

    auto* grid = new QWidget(scroll);
    auto* gridLayout = new QGridLayout(grid);
    gridLayout->setSpacing(8);
    scroll->setWidget(grid);
}

void RecentPanel::onShown()
{
    auto* scroll = findChild<QScrollArea*>();
    auto* grid   = scroll ? scroll->widget() : nullptr;
    if (!grid) return;
    auto* gridLayout = qobject_cast<QGridLayout*>(grid->layout());
    if (!gridLayout) return;
    // Clear existing
    QLayoutItem* child;
    while ((child = gridLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }
    int row = 0, col = 0;
    if (m_recent) {
        const auto files = m_recent->list();
        if (files.isEmpty()) {
            auto* empty = new QLabel(tr("No recent files."), grid);
            gridLayout->addWidget(empty, 0, 0);
            return;
        }
        for (const auto& path : files) {
            auto* card = new QPushButton(grid);
            card->setObjectName(QStringLiteral("VelaBsRecentCard"));
            card->setCursor(Qt::PointingHandCursor);
            card->setFixedSize(140, 120);
            card->setStyleSheet(QStringLiteral(
                "QPushButton#VelaBsRecentCard {"
                "  background: #FFFFFF;"
                "  border: 1px solid #D8D8E0;"
                "  border-radius: 4px;"
                "  padding: 12px;"
                "  text-align: left;"
                "}"
                "QPushButton#VelaBsRecentCard:hover {"
                "  border-color: #3525CD;"
                "}"));
            auto* cl = new QVBoxLayout(card);
            auto* thumb = new QLabel(QStringLiteral("\xF0\x9F\x93\x9D"), card);
            thumb->setStyleSheet(QStringLiteral("font-size: 32px; color: #777587;"));
            cl->addWidget(thumb);
            auto* name = new QLabel(path.section(QLatin1Char('/'), -1), card);
            name->setStyleSheet(QStringLiteral("font-size: 12px;"));
            name->setWordWrap(true);
            cl->addWidget(name);
            card->setProperty("velaPath", path);
            connect(card, &QPushButton::clicked, this, &RecentPanel::onCardClicked);
            gridLayout->addWidget(card, row, col);
            if (++col >= 4) { col = 0; row++; }
        }
    }
}

void RecentPanel::onCardClicked()
{
    auto* card = qobject_cast<QPushButton*>(sender());
    if (!card) return;
    emit fileChosen(card->property("velaPath").toString());
}

} // namespace vela