// ════════════════════════════════════════════════════════════════════════════
//  MainWindow.cpp — Stitch "Vela Precision" layout — v0.2.0
// ════════════════════════════════════════════════════════════════════════════
#include "app/MainWindow.h"
#include "app/Ribbon.h"
#include "app/TabBar.h"
#include "app/Sidebar.h"
#include "app/StatusBar.h"
#include "app/LeftRail.h"
#include "app/RecentFiles.h"
#include "app/EditorBridge.h"
#include "app/WelcomeView.h"
#include "ui/dialogs/AboutDialog.h"
#include "ui/dialogs/PageSetupDialog.h"
#include "ui/dialogs/PreferencesDialog.h"
#include "ui/dialogs/SaveChangesDialog.h"
#include "document/Document.h"
#include "document/DocumentController.h"
#include "document/TabSession.h"
#include "services/SettingsStore.h"
#include "util/Logger.h"

#ifdef VE_HAS_WEBENGINE
#  include <QWebEngineView>
#  include <QWebEnginePage>
#  include <QWebEngineProfile>
#  include <QWebEngineSettings>
#endif
#ifdef VE_HAS_WEBCHANNEL
#  include <QWebChannel>
#endif

#include <QAction>
#include <QActionGroup>
#include <QAccessibilityHints>
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEventLoop>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPageLayout>
#include <QPrintDialog>
#include <QPrinter>
#include <QPushButton>
#include <QScopeGuard>
#include <QSaveFile>
#include <QScopedValueRollback>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStyle>
#include <QStyleHints>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QWebEngineView>
#include <memory>

namespace vela {

namespace {
    constexpr int kDefaultWindowWidth  = 1280;
    constexpr int kDefaultWindowHeight = 800;
    constexpr int kMinimumWindowWidth  = 960;
    constexpr int kMinimumWindowHeight = 600;

    // Forward a format command to the embedded TipTap editor
    inline void sendFormatCmd(QWebEngineView* view, const QString& command, const QString& arg = {})
    {
#ifdef VE_HAS_WEBENGINE
        if (!view) return;
        QString js = QStringLiteral("window.__exec && window.__exec('%1'%2);")
            .arg(command, arg.isEmpty() ? QString() : QStringLiteral(", '%1'").arg(arg));
        view->page()->runJavaScript(js);
#endif
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  Construction / destruction
// ═══════════════════════════════════════════════════════════════════════════

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_draftRecovery(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/Recovery"))
{
    VE_LOG_DEBUG("MainWindow constructing");
    setObjectName(QStringLiteral("VelaMainWindow"));
    setWindowTitle(QStringLiteral("Vela Docs"));
    resize(kDefaultWindowWidth, kDefaultWindowHeight);
    setMinimumSize(kMinimumWindowWidth, kMinimumWindowHeight);
    setAcceptDrops(true);
    setDockNestingEnabled(true);
    setDockOptions(QMainWindow::AllowNestedDocks | QMainWindow::AnimatedDocks);

    m_recentFiles = new RecentFiles(this);
    m_bridge = new EditorBridge(nullptr, this);

    buildActions();
    buildMenuBar();
    buildTopBar();
    buildTabStrip();
    buildLeftRail();
    buildRibbon();
    buildSidebar();
    buildStatusBar();
    buildEditorArea();

    wireSignals();
    setupWebChannel();
    connect(QApplication::styleHints()->accessibility(), &QAccessibilityHints::contrastPreferenceChanged,
            this, &MainWindow::updateEditorAppearance);

    updateWindowTitle();
    showWelcomeView();
    m_actDarkMode->setChecked(SettingsStore().theme() == SettingsStore::Theme::Dark);

    if (!QCoreApplication::arguments().contains(QStringLiteral("--safe-mode"))) {
        auto* backup = new QTimer(this);
        backup->setInterval(5000);
        connect(backup, &QTimer::timeout, this, &MainWindow::backUpDraft);
        backup->start();
        QTimer::singleShot(0, this, &MainWindow::offerDraftRecovery);
    }

    VE_LOG_DEBUG("MainWindow constructed");
}

MainWindow::~MainWindow()
{
    VE_LOG_DEBUG("MainWindow destructing");
}

// ═══════════════════════════════════════════════════════════════════════════
//  Actions — covers File · Edit · Insert · Format · Tools · Table · Help
// ═══════════════════════════════════════════════════════════════════════════

void MainWindow::buildActions()
{
    // ── File ─────────────────────────────────────────────────────
    m_actNew = new QAction(QApplication::style()->standardIcon(QStyle::SP_FileIcon),
        tr("&New"), this);
    m_actNew->setShortcut(QKeySequence::New);
    connect(m_actNew, &QAction::triggered, this, &MainWindow::newDocument);

    m_actOpen = new QAction(QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon),
        tr("&Open…"), this);
    m_actOpen->setShortcut(QKeySequence::Open);
    connect(m_actOpen, &QAction::triggered, this, &MainWindow::openDocumentDialog);

    m_actSave = new QAction(QApplication::style()->standardIcon(QStyle::SP_DialogSaveButton),
        tr("&Save"), this);
    m_actSave->setShortcut(QKeySequence::Save);
    connect(m_actSave, &QAction::triggered, this, &MainWindow::saveDocument);

    m_actSaveAs = new QAction(tr("Save &As…"), this);
    m_actSaveAs->setShortcut(QKeySequence::SaveAs);
    connect(m_actSaveAs, &QAction::triggered, this, &MainWindow::saveDocumentAs);

    m_actClose = new QAction(tr("&Close Document"), this);
    m_actClose->setShortcut(QKeySequence::Close);
    connect(m_actClose, &QAction::triggered, this, &MainWindow::closeDocument);

    m_actQuit = new QAction(tr("&Quit"), this);
    m_actQuit->setShortcut(QKeySequence::Quit);
    m_actQuit->setMenuRole(QAction::QuitRole);
    connect(m_actQuit, &QAction::triggered, qApp, &QApplication::quit);

    // ── Edit ─────────────────────────────────────────────────────
    m_actUndo = new QAction(tr("&Undo"), this);
    m_actUndo->setShortcut(QKeySequence::Undo);
    connect(m_actUndo, &QAction::triggered, this, &MainWindow::undo);

    m_actRedo = new QAction(tr("&Redo"), this);
    m_actRedo->setShortcut(QKeySequence::Redo);
    connect(m_actRedo, &QAction::triggered, this, &MainWindow::redo);

    m_actCut = new QAction(tr("Cu&t"), this);
    m_actCut->setShortcut(QKeySequence::Cut);
    m_actCopy = new QAction(tr("&Copy"), this);
    m_actCopy->setShortcut(QKeySequence::Copy);
    m_actPaste = new QAction(tr("&Paste"), this);
    m_actPaste->setShortcut(QKeySequence::Paste);
#ifdef VE_HAS_WEBENGINE
    connect(m_actCut, &QAction::triggered, this, [this] {
        if (m_webView) m_webView->triggerPageAction(QWebEnginePage::Cut);
    });
    connect(m_actCopy, &QAction::triggered, this, [this] {
        if (m_webView) m_webView->triggerPageAction(QWebEnginePage::Copy);
    });
    connect(m_actPaste, &QAction::triggered, this, [this] {
        if (m_webView) m_webView->triggerPageAction(QWebEnginePage::Paste);
    });
#endif

    m_actFind = new QAction(tr("&Find and Replace…"), this);
    m_actFind->setShortcut(QKeySequence::Find);
    connect(m_actFind, &QAction::triggered, this, &MainWindow::findReplace);

    // ── Insert ──────────────────────────────────────────────────
    m_actInsertImage = new QAction(tr("&Image…"), this);
    m_actInsertImage->setShortcut(QKeySequence(QStringLiteral("Ctrl+Alt+I")));
    connect(m_actInsertImage, &QAction::triggered, this, [this] {
        sendFormatCmd(m_webView, QStringLiteral("openInsertImage"));
    });

    m_actInsertTable = new QAction(tr("&Table…"), this);
    m_actInsertTable->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+K")));
    connect(m_actInsertTable, &QAction::triggered, this, [this] {
        sendFormatCmd(m_webView, QStringLiteral("openInsertTable"));
    });

    m_actInsertLink = new QAction(tr("&Link…"), this);
    m_actInsertLink->setShortcut(QKeySequence(QStringLiteral("Ctrl+K")));
    connect(m_actInsertLink, &QAction::triggered, this, [this] {
        sendFormatCmd(m_webView, QStringLiteral("insertLink"));
    });

    m_actInsertPageBreak = new QAction(tr("Page &Break"), this);
    m_actInsertPageBreak->setShortcut(QKeySequence(QStringLiteral("Ctrl+Return")));
    m_actInsertPageBreak->setShortcutContext(Qt::ApplicationShortcut);
    connect(m_actInsertPageBreak, &QAction::triggered, this, [this] {
        sendFormatCmd(m_webView, QStringLiteral("pageBreak"));
    });

    m_actInsertHorizontalRule = new QAction(tr("&Horizontal Rule"), this);
    connect(m_actInsertHorizontalRule, &QAction::triggered, this, [this] {
        sendFormatCmd(m_webView, QStringLiteral("hr"));
    });


    // ── Format ──────────────────────────────────────────────────
    m_actBold = new QAction(tr("&Bold"), this);
    m_actBold->setShortcut(QKeySequence(QStringLiteral("Ctrl+B")));
    connect(m_actBold, &QAction::triggered, this, [this] {
        sendFormatCmd(m_webView, QStringLiteral("bold"));
    });

    m_actItalic = new QAction(tr("&Italic"), this);
    m_actItalic->setShortcut(QKeySequence(QStringLiteral("Ctrl+I")));
    connect(m_actItalic, &QAction::triggered, this, [this] {
        sendFormatCmd(m_webView, QStringLiteral("italic"));
    });

    m_actUnderline = new QAction(tr("&Underline"), this);
    m_actUnderline->setShortcut(QKeySequence(QStringLiteral("Ctrl+U")));
    connect(m_actUnderline, &QAction::triggered, this, [this] {
        sendFormatCmd(m_webView, QStringLiteral("underline"));
    });

    m_actStrike = new QAction(tr("&Strikethrough"), this);
    m_actStrike->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+S")));
    connect(m_actStrike, &QAction::triggered, this, [this] {
        sendFormatCmd(m_webView, QStringLiteral("strike"));
    });

    m_actClearFormatting = new QAction(tr("&Clear Formatting"), this);
    connect(m_actClearFormatting, &QAction::triggered, this, [this] {
        sendFormatCmd(m_webView, QStringLiteral("clearFormatting"));
    });

    m_actBulletList     = new QAction(tr("Bullet &List"), this);
    m_actBulletList->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+L")));
    m_actOrderedList    = new QAction(tr("&Numbered List"), this);
    m_actOrderedList->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+O")));
    m_actTaskList       = new QAction(tr("&Task List"), this);
    m_actTaskList->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+7")));
    connect(m_actBulletList,  &QAction::triggered, this, [this]{ sendFormatCmd(m_webView, QStringLiteral("bulletList")); });
    connect(m_actOrderedList, &QAction::triggered, this, [this]{ sendFormatCmd(m_webView, QStringLiteral("orderedList")); });
    connect(m_actTaskList,    &QAction::triggered, this, [this]{ sendFormatCmd(m_webView, QStringLiteral("taskList")); });

    // ── Paragraph styles (submenu-style) ───────────────────────
    QAction* actHeading = nullptr;
    auto headingAction = [&](int level, const QString& label) {
        actHeading = new QAction(label, this);
        QObject::connect(actHeading, &QAction::triggered, this, [this, level] {
            sendFormatCmd(m_webView, QStringLiteral("heading"),
                          QStringLiteral("h%1").arg(level));
        });
        return actHeading;
    };
    // Stored on the class because buildMenus() adds it to the style submenu.
    m_actNormalText = new QAction(tr("Normal text"), this);
    QObject::connect(m_actNormalText, &QAction::triggered, this, [this] {
        sendFormatCmd(m_webView, QStringLiteral("heading"), QStringLiteral("p"));
    });
    m_actHeading1 = headingAction(1, tr("Heading &1"));
    m_actHeading2 = headingAction(2, tr("Heading &2"));
    m_actHeading3 = headingAction(3, tr("Heading &3"));

    // Alignment
    m_actAlignLeft    = new QAction(tr("Align &Left"), this);
    m_actAlignCenter  = new QAction(tr("Align &Center"), this);
    m_actAlignRight   = new QAction(tr("Align &Right"), this);
    m_actAlignJustify = new QAction(tr("&Justify"), this);
    connect(m_actAlignLeft,    &QAction::triggered, this, [this]{ sendFormatCmd(m_webView, QStringLiteral("align"), QStringLiteral("left")); });
    connect(m_actAlignCenter,  &QAction::triggered, this, [this]{ sendFormatCmd(m_webView, QStringLiteral("align"), QStringLiteral("center")); });
    connect(m_actAlignRight,   &QAction::triggered, this, [this]{ sendFormatCmd(m_webView, QStringLiteral("align"), QStringLiteral("right")); });
    connect(m_actAlignJustify, &QAction::triggered, this, [this]{ sendFormatCmd(m_webView, QStringLiteral("align"), QStringLiteral("justify")); });

    m_actIndent  = new QAction(tr("&Indent"), this);
    m_actIndent->setShortcut(QKeySequence(QStringLiteral("Ctrl+]")));
    connect(m_actIndent, &QAction::triggered, this, [this]{ sendFormatCmd(m_webView,"indent"); });
    m_actOutdent = new QAction(tr("O&utdent"), this);
    m_actOutdent->setShortcut(QKeySequence(QStringLiteral("Ctrl+[")));
    connect(m_actOutdent, &QAction::triggered, this, [this]{ sendFormatCmd(m_webView,"outdent"); });

    // ── Page layout / Header & Footer (v0.5.0) ──────────────────
    m_actPageSetup = new QAction(tr("&Page Setup…"), this);
    connect(m_actPageSetup, &QAction::triggered, this, [this] {
        QJsonObject settings;
        if (!synchronizeEditor(&settings)) return;
        PageSetupDialog dlg(m_webView, this);
        dlg.setSettings(settings);
        dlg.exec();
    });
    m_actHeaderFooter = new QAction(tr("&Header and Footer…"), this);
    connect(m_actHeaderFooter, &QAction::triggered, this, [this] {
        sendFormatCmd(m_webView, QStringLiteral("editHeader"));
    });
    m_actPageNumbering = new QAction(tr("&Page Numbering…"), this);
    connect(m_actPageNumbering, &QAction::triggered, this, [this] {
        sendFormatCmd(m_webView, QStringLiteral("insertPageNumber"));
    });
    m_actOrientationPortrait = new QAction(tr("&Portrait"), this);
    m_actOrientationPortrait->setCheckable(true);
    m_actOrientationPortrait->setChecked(true);
    connect(m_actOrientationPortrait, &QAction::triggered, this, [this] {
        sendFormatCmd(m_webView, QStringLiteral("setOrientation"), QStringLiteral("portrait"));
    });
    m_actOrientationLandscape = new QAction(tr("&Landscape"), this);
    m_actOrientationLandscape->setCheckable(true);
    connect(m_actOrientationLandscape, &QAction::triggered, this, [this] {
        sendFormatCmd(m_webView, QStringLiteral("setOrientation"), QStringLiteral("landscape"));
    });
    // Orientation is an exclusive radio pair.
    auto* orientGroup = new QActionGroup(this);
    orientGroup->addAction(m_actOrientationPortrait);
    orientGroup->addAction(m_actOrientationLandscape);

    m_actPageModePageless = new QAction(tr("&Pageless (continuous)"), this);
    m_actPageModePageless->setCheckable(true);
    m_actPageModePageless->setChecked(true);
    connect(m_actPageModePageless, &QAction::triggered, this, [this] {
        sendFormatCmd(m_webView, QStringLiteral("setPageMode"), QStringLiteral("pageless"));
    });
    m_actPageModePaging = new QAction(tr("Pa&ging (A4 sheets)"), this);
    m_actPageModePaging->setCheckable(true);
    connect(m_actPageModePaging, &QAction::triggered, this, [this] {
        sendFormatCmd(m_webView, QStringLiteral("setPageMode"), QStringLiteral("paging"));
    });
    auto* modeGroup = new QActionGroup(this);
    modeGroup->addAction(m_actPageModePageless);
    modeGroup->addAction(m_actPageModePaging);

    // ── Table ───────────────────────────────────────────────────
    auto tableAction = [&](const QString& cmd, const QString& text) {
        QAction* a = new QAction(text, this);
        QObject::connect(a, &QAction::triggered, this, [this, cmd] {
            sendFormatCmd(m_webView, cmd);
        });
        return a;
    };
    m_actTblInsertRowAbove    = tableAction("addRowBefore",   tr("Insert Row &Above"));
    m_actTblInsertRowBelow    = tableAction("addRowAfter",    tr("Insert Row &Below"));
    m_actTblDeleteRow         = tableAction("deleteRow",      tr("&Delete Row"));
    m_actTblInsertColLeft     = tableAction("addColBefore",   tr("Insert Column &Left"));
    m_actTblInsertColRight    = tableAction("addColAfter",    tr("Insert Column &Right"));
    m_actTblDeleteCol         = tableAction("deleteCol",      tr("D&elete Column"));
    m_actTblMergeCells        = tableAction("mergeCells",     tr("&Merge Cells"));
    m_actTblSplitCell         = tableAction("splitCell",      tr("Sp&lit Cell"));
    m_actTblToggleHeader      = tableAction("toggleHeader",   tr("Toggle &Header Row"));
    m_actTblDeleteTable       = tableAction("deleteTable",    tr("D&elete Table"));

    // ── View ────────────────────────────────────────────────────
    m_actFocusMode = new QAction(tr("&Focus Mode"), this);
    m_actFocusMode->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+F11")));
    m_actFocusMode->setCheckable(true);
    connect(m_actFocusMode, &QAction::toggled, this, &MainWindow::toggleFocusMode);

    m_actOutline = new QAction(tr("Document &Outline"), this);
    m_actOutline->setCheckable(true);
    m_actOutline->setShortcut(QKeySequence(QStringLiteral("Ctrl+Alt+O")));
    connect(m_actOutline, &QAction::toggled, this, &MainWindow::updateEditorAppearance);

    m_actDarkMode = new QAction(tr("&Dark Mode"), this);
    m_actDarkMode->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+D")));
    m_actDarkMode->setCheckable(true);
    connect(m_actDarkMode, &QAction::toggled, this, &MainWindow::toggleDarkMode);

    m_actZoomIn = new QAction(tr("Zoom &In"), this);
    m_actZoomIn->setShortcut(QKeySequence::ZoomIn);
    connect(m_actZoomIn, &QAction::triggered, this, &MainWindow::zoomIn);
    m_actZoomOut = new QAction(tr("Zoom &Out"), this);
    m_actZoomOut->setShortcut(QKeySequence::ZoomOut);
    connect(m_actZoomOut, &QAction::triggered, this, &MainWindow::zoomOut);
    m_actZoomReset = new QAction(tr("&Reset Zoom"), this);
    m_actZoomReset->setShortcut(QKeySequence(QStringLiteral("Ctrl+0")));
    connect(m_actZoomReset, &QAction::triggered, this, &MainWindow::zoomReset);

    // ── Output ─────────────────────────────────────────────────
    m_actPrint = new QAction(tr("&Print…"), this);
    m_actPrint->setShortcut(QKeySequence::Print);
    connect(m_actPrint, &QAction::triggered, this, &MainWindow::printDocument);
    m_actPrintPreview = new QAction(tr("Print Pre&view…"), this);
    connect(m_actPrintPreview, &QAction::triggered, this, [this] {
        if (!m_printPreviews.isValid()) {
            QMessageBox::warning(this, tr("Print Preview"), tr("Could not create a temporary preview file."));
            return;
        }
        exportDocumentToPdf(m_printPreviews.filePath(tr("Print Preview %1.pdf").arg(++m_previewNumber)), true);
    });
    m_actExportPdf = new QAction(tr("Export as P&DF…"), this);
    connect(m_actExportPdf, &QAction::triggered, this, [this] {
        const QString source = currentFile();
        const QString name = source.isEmpty() ? tr("Untitled") : QFileInfo(source).completeBaseName();
        QFileDialog dialog(this, tr("Export PDF"), name + QStringLiteral(".pdf"), tr("PDF document (*.pdf)"));
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        dialog.setDefaultSuffix(QStringLiteral("pdf"));
        if (dialog.exec() == QDialog::Accepted) exportDocumentToPdf(dialog.selectedFiles().value(0));
    });

    // ── Settings ───────────────────────────────────────────────
    m_actPreferences = new QAction(tr("&Preferences…"), this);
    m_actPreferences->setShortcut(QKeySequence::Preferences);
    m_actPreferences->setMenuRole(QAction::PreferencesRole);
    connect(m_actPreferences, &QAction::triggered, this, &MainWindow::showPreferences);

    m_actWordCount = new QAction(tr("&Word Count…"), this);
    m_actWordCount->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+C")));
    connect(m_actWordCount, &QAction::triggered, this, [this] {
        if (m_bridge) {
            QVariantMap stats = m_bridge->getStats();
            QMessageBox::information(this, tr("Word Count"),
                tr("Words: %1\nCharacters: %2\nReading time: ~%3 min")
                    .arg(stats["words"].toInt())
                    .arg(stats["characters"].toInt())
                    .arg(qMax(1, stats["words"].toInt() / 200)));
        }
    });

    m_actAbout = new QAction(tr("&About Vela Docs"), this);
    m_actAbout->setMenuRole(QAction::AboutRole);
    connect(m_actAbout, &QAction::triggered, this, &MainWindow::showAbout);

    // ── Top-bar actions ────────────────────────────────────────
    m_actToggleTheme = new QAction(tr("Dark mode"), this);
    connect(m_actToggleTheme, &QAction::triggered, this, [this] {
        m_actDarkMode->toggle();
    });
}

void MainWindow::buildMenuBar()
{
    m_menuBar = new QMenuBar(this);
    m_menuBar->setObjectName(QStringLiteral("VelaMenuBar"));

    // ── File ──────────────────────────────────────────────────
    QMenu* fileMenu = m_menuBar->addMenu(tr("&File"));
    fileMenu->addAction(m_actNew);
    fileMenu->addAction(m_actOpen);
    m_menuRecent = fileMenu->addMenu(tr("Open &Recent"));
    updateRecentFilesMenu();
    fileMenu->addSeparator();
    fileMenu->addAction(m_actSave);
    fileMenu->addAction(m_actSaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction(m_actPrint);
    fileMenu->addAction(m_actPrintPreview);
    fileMenu->addAction(m_actExportPdf);
    fileMenu->addSeparator();
    fileMenu->addAction(m_actClose);
    fileMenu->addAction(m_actQuit);

    // ── Edit ──────────────────────────────────────────────────
    QMenu* editMenu = m_menuBar->addMenu(tr("&Edit"));
    editMenu->menuAction()->setProperty("editorOnly", true);
    editMenu->addAction(m_actUndo);
    editMenu->addAction(m_actRedo);
    editMenu->addSeparator();
    editMenu->addAction(m_actCut);
    editMenu->addAction(m_actCopy);
    editMenu->addAction(m_actPaste);
    editMenu->addSeparator();
    editMenu->addAction(m_actFind);

    // ── Insert ───────────────────────────────────────────────
    QMenu* insertMenu = m_menuBar->addMenu(tr("&Insert"));
    insertMenu->menuAction()->setProperty("editorOnly", true);
    insertMenu->addAction(m_actInsertImage);
    insertMenu->addAction(m_actInsertTable);
    insertMenu->addAction(m_actInsertLink);
    insertMenu->addSeparator();
    insertMenu->addAction(m_actInsertPageBreak);
    insertMenu->addAction(m_actInsertHorizontalRule);

    // ── Format ───────────────────────────────────────────────
    QMenu* formatMenu = m_menuBar->addMenu(tr("F&ormat"));
    formatMenu->menuAction()->setProperty("editorOnly", true);
    QMenu* styleMenu = formatMenu->addMenu(tr("&Paragraph Style"));
    styleMenu->addAction(m_actNormalText);
    styleMenu->addAction(m_actHeading1);
    styleMenu->addAction(m_actHeading2);
    styleMenu->addAction(m_actHeading3);
    formatMenu->addSeparator();
    formatMenu->addAction(m_actBold);
    formatMenu->addAction(m_actItalic);
    formatMenu->addAction(m_actUnderline);
    formatMenu->addAction(m_actStrike);
    formatMenu->addSeparator();
    QMenu* alignMenu = formatMenu->addMenu(tr("&Align"));
    alignMenu->addAction(m_actAlignLeft);
    alignMenu->addAction(m_actAlignCenter);
    alignMenu->addAction(m_actAlignRight);
    alignMenu->addAction(m_actAlignJustify);
    formatMenu->addSeparator();
    QMenu* listMenu = formatMenu->addMenu(tr("&Lists"));
    listMenu->addAction(m_actBulletList);
    listMenu->addAction(m_actOrderedList);
    listMenu->addAction(m_actTaskList);
    formatMenu->addSeparator();
    formatMenu->addAction(m_actIndent);
    formatMenu->addAction(m_actOutdent);
    formatMenu->addSeparator();
    formatMenu->addAction(m_actClearFormatting);
    // v0.5.0 — page layout section
    formatMenu->addSeparator();
    formatMenu->addAction(m_actPageSetup);
    formatMenu->addAction(m_actHeaderFooter);
    formatMenu->addAction(m_actPageNumbering);
    QMenu* orientSubMenu = formatMenu->addMenu(tr("&Orientation"));
    orientSubMenu->addAction(m_actOrientationPortrait);
    orientSubMenu->addAction(m_actOrientationLandscape);
    QMenu* pageModeSubMenu = formatMenu->addMenu(tr("Page &Mode"));
    pageModeSubMenu->addAction(m_actPageModePageless);
    pageModeSubMenu->addAction(m_actPageModePaging);

    // ── Table ────────────────────────────────────────────────
    QMenu* tableMenu = m_menuBar->addMenu(tr("&Table"));
    tableMenu->menuAction()->setProperty("editorOnly", true);
    tableMenu->addAction(m_actInsertTable);
    tableMenu->addSeparator();
    tableMenu->addAction(m_actTblInsertRowAbove);
    tableMenu->addAction(m_actTblInsertRowBelow);
    tableMenu->addAction(m_actTblDeleteRow);
    tableMenu->addSeparator();
    tableMenu->addAction(m_actTblInsertColLeft);
    tableMenu->addAction(m_actTblInsertColRight);
    tableMenu->addAction(m_actTblDeleteCol);
    tableMenu->addSeparator();
    tableMenu->addAction(m_actTblMergeCells);
    tableMenu->addAction(m_actTblSplitCell);
    tableMenu->addAction(m_actTblToggleHeader);
    tableMenu->addAction(m_actTblDeleteTable);

    // ── View ─────────────────────────────────────────────────
    QMenu* viewMenu = m_menuBar->addMenu(tr("&View"));
    viewMenu->addAction(m_actFocusMode);
    viewMenu->addAction(m_actOutline);
    viewMenu->addAction(m_actDarkMode);
    viewMenu->addSeparator();
    viewMenu->addAction(m_actZoomIn);
    viewMenu->addAction(m_actZoomOut);
    viewMenu->addAction(m_actZoomReset);

    // ── Tools ────────────────────────────────────────────────
    QMenu* toolsMenu = m_menuBar->addMenu(tr("&Tools"));
    toolsMenu->addAction(m_actWordCount);
    toolsMenu->addSeparator();
    toolsMenu->addAction(m_actPreferences);

    // ── Help ─────────────────────────────────────────────────
    QMenu* helpMenu = m_menuBar->addMenu(tr("&Help"));
    helpMenu->addAction(m_actAbout);
}

// ═══════════════════════════════════════════════════════════════════════════
//  Layout builders
// ═══════════════════════════════════════════════════════════════════════════

void MainWindow::buildTopBar()
{
    m_topBar = new QWidget(this);
    m_topBar->setObjectName(QStringLiteral("VelaTopBar"));

    auto* layout = new QHBoxLayout(m_topBar);
    layout->setContentsMargins(16, 8, 16, 8);
    layout->setSpacing(10);

    auto* mark = new QLabel(m_topBar);
    mark->setPixmap(qApp->windowIcon().pixmap(QSize(28, 28), devicePixelRatioF()));
    layout->addWidget(mark);

    auto* brand = new QLabel(tr("Vela Docs"), m_topBar);
    brand->setObjectName(QStringLiteral("VelaTopBarBrand"));
    layout->addWidget(brand);
    layout->addSpacing(12);
    layout->addWidget(m_menuBar);

    layout->addStretch(1);

    m_documentStatus = new QLabel(m_topBar);
    m_documentStatus->setObjectName(QStringLiteral("VelaDocumentStatus"));
    m_documentStatus->setTextFormat(Qt::PlainText);
    layout->addWidget(m_documentStatus);

    auto* themeBtn = new QToolButton(m_topBar);
    themeBtn->setObjectName(QStringLiteral("VelaThemeButton"));
    themeBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    themeBtn->setDefaultAction(m_actToggleTheme);
    layout->addWidget(themeBtn);

    setMenuWidget(m_topBar);
}

void MainWindow::buildTabStrip()
{
    // TabBar is a QTabWidget, not a QToolBar, so it cannot go through
    // addToolBar() — buildEditorArea() puts it above the editor instead.
    m_tabBar = new TabBar(this);
    m_tabBar->setManagedClose(true);
}

void MainWindow::buildLeftRail()
{
    m_leftRail = new LeftRail(this);
    addToolBar(Qt::LeftToolBarArea, m_leftRail);
}

void MainWindow::buildRibbon()
{
    m_ribbon = new Ribbon(this);
    addToolBar(Qt::TopToolBarArea, m_ribbon);
    connect(m_ribbon, &Ribbon::formatCommand,
            this, &MainWindow::onRibbonCommand);
}

void MainWindow::buildSidebar()
{
    m_sidebar = new Sidebar(this);
    m_sidebar->setObjectName(QStringLiteral("VelaSidebar"));
    addDockWidget(Qt::RightDockWidgetArea, m_sidebar);
}

void MainWindow::buildStatusBar()
{
    m_statusBar = new StatusBar(this);
    m_statusBar->setObjectName(QStringLiteral("VelaStatusBar"));
    setStatusBar(m_statusBar);
    m_statusBar->setReadyMessage(tr("Ready"));
}

void MainWindow::buildEditorArea()
{
    m_centralStack = new QStackedWidget(this);

    m_welcomeView = new WelcomeView(m_centralStack);
    m_welcomeView->setRecentFiles(m_recentFiles->list());
    connect(m_welcomeView, &WelcomeView::newRequested, this, &MainWindow::newDocument);
    connect(m_welcomeView, &WelcomeView::openRequested, this, &MainWindow::openDocumentDialog);
    connect(m_welcomeView, &WelcomeView::recentRequested, this, &MainWindow::openRecentDocument);
    m_centralStack->addWidget(m_welcomeView);

#ifdef VE_HAS_WEBENGINE
    m_webView = new QWebEngineView(this);
    m_webView->setObjectName(QStringLiteral("VelaEditorCanvas"));
    auto* profile = m_webView->page()->profile();
    profile->setSpellCheckLanguages({QStringLiteral("en-US")});
    profile->setSpellCheckEnabled(SettingsStore().spellCheckEnabled());
    QWebEngineSettings* s = m_webView->page()->settings();
    s->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    s->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    s->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, true);

    // Tab strip directly above the canvas, inside the same stack page.
    QWidget* editorPage = new QWidget(this);
    editorPage->setObjectName(QStringLiteral("VelaEditorPage"));
    QVBoxLayout* editorLayout = new QVBoxLayout(editorPage);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(0);
    if (m_tabBar) editorLayout->addWidget(m_tabBar);
    editorLayout->addWidget(m_webView, 1);
    m_centralStack->addWidget(editorPage);
#else
    QLabel* placeholder = new QLabel(tr("Qt WebEngine not available.\n\nBuild with Qt6WebEngine to enable the TipTap editor."), m_welcomeView);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setStyleSheet(QStringLiteral("color: #777587; font-size: 16px;"));
    m_centralStack->addWidget(placeholder);
#endif

    setCentralWidget(m_centralStack);
}

namespace {
/// Resolve the editor URL. In production this is the offline-bundled
/// `qrc:/editor/index.html`. With `--dev` or `VELA_DEV=1` it points at the
/// Vite dev server (`http://localhost:5173/`) so the Qt app can hot-reload
/// the editor UI without rebuilding C++.
QUrl editorUrlForRuntime()
{
    auto args = QCoreApplication::arguments();
    QString envUrl = qEnvironmentVariable("VELA_DEV_URL");
    if (!envUrl.isEmpty())
        return QUrl(envUrl);

    bool dev = qEnvironmentVariableIntValue("VELA_DEV") > 0;
    for (const QString& arg : args) {
        if (arg == QStringLiteral("--dev") || arg == QStringLiteral("-d"))
            dev = true;
        else if (arg.startsWith(QStringLiteral("--dev-url=")))
            return QUrl(arg.mid(QStringLiteral("--dev-url=").size()));
    }
    if (dev)
        return QUrl(QStringLiteral("http://localhost:5173/"));
    return QUrl(QStringLiteral("qrc:/editor/index.html"));
}
} // anon

void MainWindow::setupWebChannel()
{
#ifdef VE_HAS_WEBCHANNEL
#  ifdef VE_HAS_WEBENGINE
    if (!m_webView || !m_bridge) return;

    m_webChannel = new QWebChannel(this);
    m_webChannel->registerObject(QStringLiteral("vela"), m_bridge);
    m_webView->page()->setWebChannel(m_webChannel);
    connect(m_webView, &QWebEngineView::loadFinished, this, &MainWindow::updateEditorAppearance);

    const QUrl url = editorUrlForRuntime();
    VE_LOG_INFO(QStringLiteral("MainWindow: editor URL = %1").arg(url.toString()));
    m_webView->setUrl(url);

    connect(m_bridge, &EditorBridge::statsChanged,
            this, &MainWindow::onEditorStatsChanged);
    connect(m_bridge, &EditorBridge::pageSetupRequested, m_actPageSetup, &QAction::trigger,
            Qt::QueuedConnection);
    connect(m_bridge, &EditorBridge::errorReported, this,
            [this](const QString& where, const QString& msg) {
                QMessageBox::warning(this, tr("Vela Docs"),
                                     QStringLiteral("%1: %2").arg(where, msg));
            });
#  endif
#endif
}

void MainWindow::wireSignals()
{
    connect(m_recentFiles, &RecentFiles::changed,
            this, &MainWindow::updateRecentFilesMenu);
    connect(m_recentFiles, &RecentFiles::changed, m_welcomeView, [this] {
        m_welcomeView->setRecentFiles(m_recentFiles->list());
    });
    connect(m_tabBar, &TabBar::closeRequested, this, &MainWindow::closeSession);
    connect(m_tabBar, &TabBar::newTabRequested, this, &MainWindow::newDocument);
    connect(m_tabBar, &TabBar::currentSessionChanged, this, [this](TabSession* session) {
        if (session == m_session) return;
        if (m_printing || !synchronizeEditor()) {
            const QSignalBlocker blocker(m_tabBar);
            m_tabBar->setCurrentIndex(m_tabBar->indexOfSession(m_session));
            return;
        }
        activateSession(session);
    });

    // LeftRail::View is a scoped enum, so the lambdas must take it by value and
    // convert explicitly — Sidebar::showView() wants a plain int index.
    connect(m_leftRail, &LeftRail::viewActivated, this,
            [this](LeftRail::View v) {
                if (!m_sidebar) return;
                m_sidebar->show();
                m_sidebar->showView(static_cast<int>(v));
            });
    connect(m_leftRail, &LeftRail::viewDeactivated, this,
            [this](LeftRail::View) { if (m_sidebar) m_sidebar->hide(); });
}

// ═══════════════════════════════════════════════════════════════════════════
//  Helpers
// ═══════════════════════════════════════════════════════════════════════════

void MainWindow::updateWindowTitle()
{
    QString title = QStringLiteral("Vela Docs");
    const auto* document = m_controller ? m_controller->activeDocument() : nullptr;
    if (document) {
        const QString path = currentFile();
        const QString name = path.isEmpty() ? tr("Untitled") : QFileInfo(path).fileName();
        const QString state = m_saving ? tr("Saving…")
            : (path.isEmpty() || document->isDirty()) ? tr("Unsaved") : tr("Saved");
        title = tr("%1 — %2 — Vela Docs").arg(name, state);
        const QString shortName = m_documentStatus->fontMetrics().elidedText(name, Qt::ElideMiddle, 130);
        m_documentStatus->setText(tr("%1 · %2").arg(shortName, state));
        m_documentStatus->setAccessibleName(tr("%1 · %2").arg(name, state));
    }
    m_documentStatus->setVisible(document != nullptr);
    setWindowTitle(title);
}

void MainWindow::updateRecentFilesMenu()
{
    if (!m_menuRecent) return;
    m_menuRecent->clear();
    const QStringList recent = m_recentFiles->list();
    if (recent.isEmpty()) {
        QAction* empty = m_menuRecent->addAction(tr("(empty)"));
        empty->setEnabled(false);
        return;
    }
    for (const QString& path : recent) {
        const QFileInfo info(path);
        QString label = tr("%1 — %2").arg(info.fileName(), QDir::toNativeSeparators(info.absolutePath()));
        QAction* act = m_menuRecent->addAction(label.replace(QLatin1Char('&'), QStringLiteral("&&")));
        act->setToolTip(path);
        connect(act, &QAction::triggered, this, [this, path]() { openRecentDocument(path); });
    }
    m_menuRecent->addSeparator();
    QAction* clear = m_menuRecent->addAction(tr("Clear List"));
    connect(clear, &QAction::triggered, m_recentFiles, &RecentFiles::clear);
}

void MainWindow::showWelcomeView()
{
    if (m_centralStack) m_centralStack->setCurrentIndex(0);
    updateWorkspaceChrome();
}

void MainWindow::showEditorView()
{
    if (m_centralStack && m_centralStack->count() > 1)
        m_centralStack->setCurrentIndex(1);
    updateWorkspaceChrome();
    if (m_webView) m_webView->setFocus();
}

void MainWindow::updateWorkspaceChrome()
{
    const bool editing = m_centralStack->currentIndex() == 1;
    for (auto* action : m_menuBar->actions()) {
        if (action->property("editorOnly").toBool())
            action->setVisible(editing);
    }
    m_actSave->setEnabled(editing);
    m_actSaveAs->setEnabled(editing);
    m_actClose->setEnabled(editing);
    m_actPrint->setEnabled(editing && m_webView && !m_printing);
    m_actPrintPreview->setEnabled(editing && m_webView && !m_printing);
    m_actExportPdf->setEnabled(editing && m_webView && !m_printing);
    m_actFocusMode->setEnabled(editing);
    m_actOutline->setEnabled(editing);
    m_actZoomIn->setEnabled(editing);
    m_actZoomOut->setEnabled(editing);
    m_actZoomReset->setEnabled(editing);

    // The embedded editor already supplies its own ribbon and status bar.
    const bool showNativeTools = editing && !m_focusMode && !m_webView;
    m_ribbon->setVisible(showNativeTools);
    m_leftRail->setVisible(showNativeTools);
    m_statusBar->setVisible(showNativeTools);
    if (!showNativeTools) m_sidebar->hide();
    m_tabBar->setVisible(editing && !m_focusMode && m_tabBar->count() > 1);
}

bool MainWindow::canOpenFromShell() const
{
    return !m_waitingForEditor && !m_saving && !m_printing && !m_closingWindow;
}

bool MainWindow::openDocument(const QString& filePath)
{
    if (m_printing || !synchronizeEditor()) return false;
    const QFileInfo requested(filePath);
    for (int index = 0; index < m_tabBar->count(); ++index) {
        const auto* session = m_tabBar->sessionAt(index);
        if (!session->filePath().isEmpty() && QFileInfo(session->filePath()) == requested) {
            m_tabBar->setCurrentIndex(index);
            return m_session == session;
        }
    }

    std::unique_ptr<TabSession> session(createSession());
    if (!session->load(requested.absoluteFilePath())) return false;
    addSession(session.release());
    m_recentFiles->add(filePath);
    return true;
}

QString MainWindow::currentFile() const
{
    return m_session ? m_session->filePath() : QString();
}

TabSession* MainWindow::createSession()
{
    auto* session = new TabSession(this);
    connect(session->controller(), &DocumentController::errorOccurred, this, [this](const QString& message) {
        QMessageBox::warning(this, tr("Vela Docs"), message);
    });
    return session;
}

void MainWindow::addSession(TabSession* session)
{
    const QSignalBlocker blocker(m_tabBar);
    m_tabBar->addSession(session);
    activateSession(session);
}

void MainWindow::activateSession(TabSession* session)
{
    if (m_controller) disconnect(m_controller, &DocumentController::dirtyStateChanged, this, &MainWindow::onDirtyStateChanged);
    m_session = session;
    m_controller = session ? session->controller() : nullptr;
    if (m_controller) connect(m_controller, &DocumentController::dirtyStateChanged, this, &MainWindow::onDirtyStateChanged);
    m_bridge->bindSession(session);
    m_bridge->bindController(m_controller);
    if (session) showEditorView();
    else {
        m_actFocusMode->setChecked(false);
        showWelcomeView();
    }
    updateWindowTitle();
}

void MainWindow::openRecentDocument(const QString& path)
{
    const QFileInfo info(path);
    if (info.isFile()) {
        openDocument(path);
        return;
    }

    QMessageBox missing(QMessageBox::Question, tr("Document moved or unavailable"),
                        tr("Vela Docs couldn't find %1.").arg(info.fileName()), QMessageBox::Cancel, this);
    missing.setTextFormat(Qt::PlainText);
    auto* locate = missing.addButton(tr("Locate…"), QMessageBox::ActionRole);
    auto* remove = missing.addButton(tr("Remove from Recent"), QMessageBox::DestructiveRole);
    missing.exec();
    if (missing.clickedButton() == remove) m_recentFiles->remove(path);
    if (missing.clickedButton() == locate) {
        const QString replacement = QFileDialog::getOpenFileName(this, tr("Locate document"),
            info.absolutePath(), tr("Word documents (*.docx)"));
        if (!replacement.isEmpty() && openDocument(replacement)) m_recentFiles->remove(path);
    }
}

bool MainWindow::saveDocument()
{
    if (!m_controller || !m_controller->activeDocument()) return false;
    const QString path = currentFile();
    return path.isEmpty() ? saveDocumentAs() : saveDocumentTo(path);
}

bool MainWindow::saveDocumentAs()
{
    if (!m_controller || !m_controller->activeDocument() || m_waitingForEditor) return false;
    const QString suggestion = m_recoveredNames.value(m_session->document()->id(), QStringLiteral("Untitled.docx"));
    QFileDialog dialog(this, tr("Save Document As"),
        currentFile().isEmpty() ? suggestion : currentFile(),
        tr("Word Document (*.docx)"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix(QStringLiteral("docx"));
    return dialog.exec() == QDialog::Accepted && saveDocumentTo(dialog.selectedFiles().value(0));
}

bool MainWindow::saveDocumentTo(const QString& path)
{
    if (m_saving || !m_session || path.isEmpty()) return false;
    const QFileInfo destination(path);
    for (int index = 0; index < m_tabBar->count(); ++index) {
        const auto* session = m_tabBar->sessionAt(index);
        if (session != m_session && !session->filePath().isEmpty() && QFileInfo(session->filePath()) == destination) {
            QMessageBox::warning(this, tr("Document already open"), tr("Choose another filename or close the tab that already uses this file."));
            return false;
        }
    }
    m_saving = true;
    updateWindowTitle();
    const auto finished = qScopeGuard([this] {
        m_saving = false;
        updateWindowTitle();
    });
    if (!synchronizeEditor() || !m_session->saveAs(destination.absoluteFilePath())) return false;
    clearBackup(m_session);
    m_recentFiles->add(destination.absoluteFilePath());
    return true;
}

bool MainWindow::preparePrintLayout(QPageLayout& layout)
{
#ifdef VE_HAS_WEBENGINE
    if (m_printing || !m_webView || !m_session) return false;
    m_webView->page()->runJavaScript(QStringLiteral("window.__velaPreparePrint?.()"));
    QJsonObject settings;
    if (!synchronizeEditor(&settings)) return false;
    QPageSize::PageSizeId size = QPageSize::A4;
    const auto name = settings.value(QStringLiteral("size")).toString();
    if (name == QLatin1String("Letter")) size = QPageSize::Letter;
    if (name == QLatin1String("Legal")) size = QPageSize::Legal;
    if (name == QLatin1String("Tabloid")) size = QPageSize::Tabloid;
    const bool landscape = settings.value(QStringLiteral("orientation")).toString() == QLatin1String("landscape");
    const QMarginsF margins(settings.value(QStringLiteral("marginLeft")).toDouble(1),
        settings.value(QStringLiteral("marginTop")).toDouble(1),
        settings.value(QStringLiteral("marginRight")).toDouble(1),
        settings.value(QStringLiteral("marginBottom")).toDouble(1));
    layout = QPageLayout(QPageSize(size), landscape ? QPageLayout::Landscape : QPageLayout::Portrait,
                         margins, QPageLayout::Inch);
    return true;
#else
    Q_UNUSED(layout);
    return false;
#endif
}

void MainWindow::exportDocumentToPdf(const QString& path, bool preview)
{
#ifdef VE_HAS_WEBENGINE
    QPageLayout layout;
    if (path.isEmpty() || !preparePrintLayout(layout)) return;
    m_printing = true;
    updateWorkspaceChrome();
    const QPointer<MainWindow> window(this);
    m_webView->page()->printToPdf([window, path, preview](const QByteArray& pdf) {
        if (!window) return;
        window->m_printing = false;
        window->updateWorkspaceChrome();
        QSaveFile file(path);
        if (pdf.isEmpty() || !file.open(QIODevice::WriteOnly) || file.write(pdf) != pdf.size() || !file.commit()) {
            QMessageBox::warning(window, tr("PDF export failed"), tr("Vela Docs could not write the PDF."));
        } else if (preview && !QDesktopServices::openUrl(QUrl::fromLocalFile(path))) {
            QMessageBox::warning(window, tr("Print Preview"), tr("Windows could not open the PDF preview."));
        }
    }, layout);
#else
    Q_UNUSED(path);
    Q_UNUSED(preview);
#endif
}

void MainWindow::printDocument()
{
#ifdef VE_HAS_WEBENGINE
    QPageLayout layout;
    if (!preparePrintLayout(layout)) return;
    auto printer = std::make_shared<QPrinter>(QPrinter::HighResolution);
    printer->setPageLayout(layout);
    QPrintDialog dialog(printer.get(), this);
    if (dialog.exec() != QDialog::Accepted) return;
    m_printing = true;
    updateWorkspaceChrome();
    connect(m_webView, &QWebEngineView::printFinished, this, [this, printer](bool ok) {
        m_printing = false;
        updateWorkspaceChrome();
        if (!ok) QMessageBox::warning(this, tr("Print failed"), tr("Vela Docs could not print the document."));
    }, Qt::SingleShotConnection);
    m_webView->print(printer.get());
#endif
}

bool MainWindow::synchronizeEditor(QJsonObject* pageSettings, bool reportFailure)
{
#ifdef VE_HAS_WEBENGINE
    if (m_waitingForEditor) return false;
    if (!m_webView || !m_controller || !m_controller->activeDocument()) return true;

    const QScopedValueRollback waiting(m_waitingForEditor, true);
    const QPointer<Document> document = m_controller->activeDocument();
    QVariant snapshot;
    QEventLoop loop;
    const QPointer<QEventLoop> pending(&loop);
    QTimer timeout;
    timeout.setSingleShot(true);
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeout.start(2000);
    m_webView->page()->runJavaScript(QStringLiteral(
        "window.__velaEditorSnapshot ? window.__velaEditorSnapshot() : {loading: true}"),
        [pending, &snapshot](const QVariant& result) {
            if (!pending) return;
            snapshot = result;
            pending->quit();
        });
    loop.exec(QEventLoop::ExcludeUserInputEvents);
    const auto result = snapshot.toMap();
    if (!m_controller || !document || document != m_controller->activeDocument()) return false;
    // Until the channel has loaded the active document, the canvas is read-only.
    if (result.value(QStringLiteral("loading")).toBool()) return true;
    if (result.value(QStringLiteral("documentId")).toString() != document->id()) {
        if (reportFailure) QMessageBox::warning(this, tr("Editor unavailable"),
            tr("Vela Docs could not read the current edits. Try again in a moment."));
        return false;
    }
    if (result.contains(QStringLiteral("body")))
        m_bridge->setBody(result.value(QStringLiteral("body")).toMap());
    if (pageSettings) *pageSettings = QJsonObject::fromVariantMap(result.value(QStringLiteral("pageSettings")).toMap());
#endif
    return true;
}

bool MainWindow::confirmDocumentClose(TabSession* session, const QString& prompt)
{
    if (!session) return true;
    if (session == m_session && !synchronizeEditor()) return false;
    if (!session->isDirty()) return true;

    SaveChangesDialog dialog(this, prompt, session->title());
    const int result = dialog.exec();
    if (result == SaveChangesDialog::Discard) return true;
    if (result != SaveChangesDialog::Save) return false;
    m_tabBar->setCurrentIndex(m_tabBar->indexOfSession(session));
    return m_session == session && saveDocument();
}

void MainWindow::backUpDraft()
{
    if (m_saving || QApplication::activeModalWidget() || !SettingsStore().autoSaveEnabled()) return;
    synchronizeEditor(nullptr, false);
    for (int index = 0; index < m_tabBar->count(); ++index) {
        const auto* session = m_tabBar->sessionAt(index);
        if (session->isDirty() && !m_draftRecovery.store(*session->document()))
            VE_LOG_WARNING("Unable to write a recovery copy of a document");
    }
}

void MainWindow::clearBackup(TabSession* session)
{
    const QString id = session->document()->id();
    m_draftRecovery.clear(id);
    m_recoveredNames.remove(id);
}

void MainWindow::offerDraftRecovery()
{
    for (const auto& draft : m_draftRecovery.findOrphaned()) {
        const QString name = draft.originalPath.isEmpty() ? tr("Untitled") : QFileInfo(draft.originalPath).fileName();
        QMessageBox prompt(QMessageBox::Question, tr("Recover a draft"),
            tr("Recover edits to %1 from %2?").arg(name, QLocale().toString(draft.savedAt, QLocale::ShortFormat)),
            QMessageBox::Cancel, this);
        prompt.setTextFormat(Qt::PlainText);
        prompt.button(QMessageBox::Cancel)->setText(tr("Later"));
        auto* recover = prompt.addButton(tr("Recover Copy"), QMessageBox::AcceptRole);
        auto* discard = prompt.addButton(tr("Discard Draft"), QMessageBox::DestructiveRole);
        prompt.setDefaultButton(recover);
        prompt.exec();
        if (prompt.clickedButton() == discard) {
            DraftRecovery::removeSnapshot(draft);
            continue;
        }
        if (prompt.clickedButton() != recover) return;
        if (!synchronizeEditor()) return;
        auto* session = createSession();
        session->controller()->restoreDraft(draft.body);
        const QString suggestion = draft.originalPath.isEmpty() ? tr("Recovered draft.docx") : tr("Recovered - %1").arg(name);
        m_recoveredNames.insert(session->document()->id(), suggestion);
        if (m_draftRecovery.store(*session->document())) DraftRecovery::removeSnapshot(draft);
        addSession(session);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  Slots
// ═══════════════════════════════════════════════════════════════════════════

void MainWindow::newDocument()
{
    if (m_printing || !synchronizeEditor()) return;
    auto* session = createSession();
    session->controller()->newDocument();
    addSession(session);
}

void MainWindow::openDocumentDialog()
{
    const QString path = QFileDialog::getOpenFileName(this,
        tr("Open Document"), QString(), tr("Word documents (*.docx)"));
    if (!path.isEmpty()) openDocument(path);
}

void MainWindow::closeDocument()
{
    closeSession(m_session);
}

void MainWindow::closeSession(TabSession* session)
{
    if (m_printing || m_saving || !session || !confirmDocumentClose(session, tr("Save changes before closing?"))) return;
    const QString id = session->document()->id();
    clearBackup(session);
    const QSignalBlocker blocker(m_tabBar);
    m_tabBar->removeSession(m_tabBar->indexOfSession(session));
    if (session == m_session) activateSession(m_tabBar->currentSession());
    else updateWorkspaceChrome();
    m_bridge->forgetDocument(id);
    session->deleteLater();
}

void MainWindow::undo()  { sendFormatCmd(m_webView, QStringLiteral("undo")); }
void MainWindow::redo()  { sendFormatCmd(m_webView, QStringLiteral("redo")); }

void MainWindow::findReplace()
{
#ifdef VE_HAS_WEBENGINE
    // The real find/replace UI lives in the editor: a ProseMirror plugin does
    // the match highlighting and the "n of m" counter, and the bar it drives is
    // mounted by main.js. The native FindReplaceDialog this used to show was an
    // empty QDialog that could not search anything.
    if (m_webView && m_webView->page()) {
        m_webView->page()->runJavaScript(QStringLiteral(
            "window.__editor && window.__editor.commands.focus();"
            "window.__findBar && window.__findBar.open(true);"));
        return;
    }
#endif
    QMessageBox::information(this, tr("Find && Replace"),
        tr("Find and replace needs the editor to be loaded."));
}

void MainWindow::toggleFocusMode()
{
    m_focusMode = m_actFocusMode->isChecked();
    updateWorkspaceChrome();
    updateEditorAppearance();
}

void MainWindow::toggleDarkMode()
{
    m_darkMode = m_actDarkMode->isChecked();
    SettingsStore().setTheme(m_darkMode ? SettingsStore::Theme::Dark : SettingsStore::Theme::Light);
    setProperty("dark", m_darkMode);
    QApplication::styleHints()->setColorScheme(m_darkMode ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);
    m_actToggleTheme->setText(m_darkMode ? tr("Light mode") : tr("Dark mode"));
    for (auto* widget : findChildren<QWidget*>()) {
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
        widget->update();
    }
    style()->unpolish(this);
    style()->polish(this);
    update();
    updateEditorAppearance();
    if (m_statusBar) m_statusBar->showMessage(
        m_darkMode ? tr("Dark mode on") : tr("Dark mode off"), 2000);
}

void MainWindow::updateEditorAppearance()
{
#ifdef VE_HAS_WEBENGINE
    if (m_webView) {
        const bool highContrast = QApplication::styleHints()->accessibility()->contrastPreference()
            == Qt::ContrastPreference::HighContrast;
        m_webView->page()->runJavaScript(QStringLiteral(
            "document.documentElement.dataset.theme = '%1';"
            "document.documentElement.classList.toggle('vela-editor-focus', %2);"
            "window.__velaToggleOutline?.(%3);"
            "document.documentElement.classList.toggle('vela-high-contrast', %4);")
            .arg(m_darkMode ? QStringLiteral("dark") : QStringLiteral("light"),
                 m_focusMode ? QStringLiteral("true") : QStringLiteral("false"),
                 m_actOutline->isChecked() && !m_focusMode ? QStringLiteral("true") : QStringLiteral("false"),
                 highContrast ? QStringLiteral("true") : QStringLiteral("false")));
    }
#endif
}

void MainWindow::zoomIn()    { if (m_webView) m_webView->setZoomFactor(m_webView->zoomFactor() * 1.1); }
void MainWindow::zoomOut()   { if (m_webView) m_webView->setZoomFactor(m_webView->zoomFactor() / 1.1); }
void MainWindow::zoomReset() { if (m_webView) m_webView->setZoomFactor(1.0); }

void MainWindow::about()
{
    AboutDialog dlg(this);
    dlg.exec();
}

void MainWindow::showPreferences()
{
    SettingsStore settings;
    PreferencesDialog dialog(this);
    dialog.setOptions(m_darkMode, settings.autoSaveEnabled(), settings.spellCheckEnabled());
    if (dialog.exec() != QDialog::Accepted) return;
    m_actDarkMode->setChecked(dialog.darkMode());
    settings.setAutoSaveEnabled(dialog.autoSaveEnabled());
    settings.setSpellCheckEnabled(dialog.spellCheckEnabled());
#ifdef VE_HAS_WEBENGINE
    if (m_webView) m_webView->page()->profile()->setSpellCheckEnabled(dialog.spellCheckEnabled());
#endif
}

void MainWindow::showAbout()
{
    about();
}

void MainWindow::onDirtyStateChanged(bool /*dirty*/)
{
    updateWindowTitle();
}

void MainWindow::onLeftRailViewActivated(int view)
{
    Q_UNUSED(view);
}

void MainWindow::onLeftRailViewDeactivated(int view)
{
    Q_UNUSED(view);
}

void MainWindow::onRibbonCommand(const QString& command)
{
    VE_LOG_DEBUG("Ribbon command: " << command);
    sendFormatCmd(m_webView, QStringLiteral("__exec"), command);
    // Actually — ribbons already call exec via window.__exec. Skip.
    (void)command;
}

void MainWindow::onEditorStatsChanged(int words, int characters, int /*pages*/)
{
    if (m_statusBar) m_statusBar->setWordCount(words, characters);
}

// ═══════════════════════════════════════════════════════════════════════════
//  Drag-drop & close
// ═══════════════════════════════════════════════════════════════════════════

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
    else event->ignore();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    if (!urls.isEmpty()) {
        const QString localPath = urls.first().toLocalFile();
        if (!localPath.isEmpty()) {
            if (openDocument(localPath)) event->acceptProposedAction();
        }
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    const QScopedValueRollback closing(m_closingWindow, true);
    event->ignore();
    if (m_printing || m_saving || !synchronizeEditor()) return;
    for (int index = 0; index < m_tabBar->count(); ++index) {
        if (!confirmDocumentClose(m_tabBar->sessionAt(index), tr("Save changes before quitting?"))) return;
    }
    for (int index = 0; index < m_tabBar->count(); ++index) clearBackup(m_tabBar->sessionAt(index));
    event->accept();
}

} // namespace vela
