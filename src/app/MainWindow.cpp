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
#include "ui/dialogs/AboutDialog.h"
#include "ui/dialogs/PageSetupDialog.h"
#include "document/Document.h"
#include "document/DocumentController.h"
#include "util/Logger.h"

#ifdef VE_HAS_WEBENGINE
#  include <QWebEngineView>
#  include <QWebEnginePage>
#  include <QWebEngineSettings>
#endif
#ifdef VE_HAS_WEBCHANNEL
#  include <QWebChannel>
#endif

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyle>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QWebEngineView>

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
{
    VE_LOG_DEBUG("MainWindow constructing");
    setObjectName(QStringLiteral("VelaMainWindow"));
    setWindowTitle(QStringLiteral("Vela Docs"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("text-editor"),
        QApplication::style()->standardIcon(QStyle::SP_FileIcon)));
    resize(kDefaultWindowWidth, kDefaultWindowHeight);
    setMinimumSize(kMinimumWindowWidth, kMinimumWindowHeight);
    setAcceptDrops(true);
    setDockNestingEnabled(true);
    setDockOptions(QMainWindow::AllowNestedDocks | QMainWindow::AnimatedDocks);

    m_recentFiles = new RecentFiles(this);
    m_controller  = new DocumentController(this);
    m_bridge      = new EditorBridge(m_controller, this);

    connect(m_controller.data(), &DocumentController::activeDocumentChanged,
            this, &MainWindow::onActiveDocumentChanged);
    connect(m_controller.data(), &DocumentController::dirtyStateChanged,
            this, &MainWindow::onDirtyStateChanged);
    connect(m_controller.data(), &DocumentController::errorOccurred, this,
            [this](const QString& msg) {
                QMessageBox::warning(this, tr("Vela Docs"), msg);
            });

    buildActions();
    buildMenuBar();   // ← now richer (File / Edit / Insert / Format / Tools / Table / Help)
    buildTopBar();
    buildTabStrip();
    buildLeftRail();
    buildRibbon();
    buildSidebar();
    buildStatusBar();
    buildEditorArea();

    wireSignals();
    setupWebChannel();

    updateWindowTitle();
    showWelcomeView();

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

    m_actInsertFootnote = new QAction(tr("&Footnote"), this);
    connect(m_actInsertFootnote, &QAction::triggered, this, [this] {
        sendFormatCmd(m_webView, QStringLiteral("footnote"));
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
        // Instantiate on demand (matches the codebase pattern for stub dialogs).
        // The dialog reads current settings, applies changes to the web editor
        // via window.__velaSetPageSettings() through QWebChannel.
        PageSetupDialog dlg(m_webView, this);
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

    m_actDarkMode = new QAction(tr("&Dark Mode"), this);
    m_actDarkMode->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+D")));
    m_actDarkMode->setCheckable(true);
    connect(m_actDarkMode, &QAction::toggled, this, &MainWindow::toggleDarkMode);

    m_actZoomIn = new QAction(tr("Zoom &In"), this);
    m_actZoomIn->setShortcut(QKeySequence::ZoomIn);
    m_actZoomOut = new QAction(tr("Zoom &Out"), this);
    m_actZoomOut->setShortcut(QKeySequence::ZoomOut);
    m_actZoomReset = new QAction(tr("&Reset Zoom"), this);
    m_actZoomReset->setShortcut(QKeySequence(QStringLiteral("Ctrl+0")));

    // ── Output ─────────────────────────────────────────────────
    m_actPrint = new QAction(tr("&Print…"), this);
    m_actPrint->setShortcut(QKeySequence::Print);
    m_actExportPdf = new QAction(tr("Export as P&DF…"), this);

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
    m_actToggleTheme = new QAction(
        QApplication::style()->standardIcon(QStyle::SP_FileDialogInfoView),
        tr("Toggle Theme"), this);
    m_actToggleTheme->setToolTip(tr("Toggle theme"));
    connect(m_actToggleTheme, &QAction::triggered, this, &MainWindow::toggleDarkMode);
}

void MainWindow::buildMenuBar()
{
    m_menuBar = menuBar();
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
    fileMenu->addAction(m_actExportPdf);
    fileMenu->addSeparator();
    fileMenu->addAction(m_actClose);
    fileMenu->addAction(m_actQuit);

    // ── Edit ──────────────────────────────────────────────────
    QMenu* editMenu = m_menuBar->addMenu(tr("&Edit"));
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
    insertMenu->addAction(m_actInsertImage);
    insertMenu->addAction(m_actInsertTable);
    insertMenu->addAction(m_actInsertLink);
    insertMenu->addSeparator();
    insertMenu->addAction(m_actInsertPageBreak);
    insertMenu->addAction(m_actInsertHorizontalRule);
    insertMenu->addAction(m_actInsertFootnote);

    // ── Format ───────────────────────────────────────────────
    QMenu* formatMenu = m_menuBar->addMenu(tr("F&ormat"));
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
    m_topBar->setFixedHeight(48);

    QHBoxLayout* layout = new QHBoxLayout(m_topBar);
    layout->setContentsMargins(0, 0, 16, 0);
    layout->setSpacing(12);

    QLabel* logo = new QLabel(QStringLiteral("Vela Docs"), m_topBar);
    QFont logoFont;
    logoFont.setPointSize(14);
    logoFont.setWeight(QFont::DemiBold);
    logo->setFont(logoFont);
    logo->setStyleSheet(QStringLiteral("color: #3525cd;"));
    layout->addWidget(logo);

    QLabel* version = new QLabel(QStringLiteral("v" VE_VERSION), m_topBar);
    version->setStyleSheet(
        QStringLiteral("color: #464555; background: #e1e8fd; padding: 2px 8px; "
                       "border-radius: 4px; font-size: 11px; font-weight: 500;"));
    layout->addWidget(version);

    layout->addStretch(1);

    QToolButton* themeBtn = new QToolButton(m_topBar);
    themeBtn->setDefaultAction(m_actToggleTheme);
    themeBtn->setIconSize(QSize(20, 20));
    themeBtn->setAutoRaise(true);
    layout->addWidget(themeBtn);

    // v0.4.2 - removed redundant Preferences button (lives in Backstage Options).

    setMenuWidget(m_topBar);
}

void MainWindow::buildTabStrip()
{
    // TabBar is a QTabWidget, not a QToolBar, so it cannot go through
    // addToolBar() — buildEditorArea() puts it above the editor instead.
    m_tabBar = new TabBar(this);
    m_tabBar->setObjectName(QStringLiteral("VelaTabBar"));
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

    // Welcome view — same as before
    m_welcomeView = new QWidget(this);
    m_welcomeView->setObjectName(QStringLiteral("VelaWelcome"));
    QVBoxLayout* welcomeLayout = new QVBoxLayout(m_welcomeView);
    welcomeLayout->setAlignment(Qt::AlignCenter);
    welcomeLayout->setContentsMargins(48, 48, 48, 48);
    welcomeLayout->setSpacing(32);

    QLabel* hero = new QLabel(QStringLiteral("A modern word processor that speaks your language."), m_welcomeView);
    hero->setObjectName(QStringLiteral("VelaWelcomeHero"));
    hero->setAlignment(Qt::AlignCenter);
    hero->setWordWrap(true);
    welcomeLayout->addWidget(hero);

    QLabel* tagline = new QLabel(
        tr("Create, edit, and collaborate with seamless precision. Vela Docs is designed to keep your focus entirely on the words."),
        m_welcomeView);
    tagline->setObjectName(QStringLiteral("VelaWelcomeTagline"));
    tagline->setAlignment(Qt::AlignCenter);
    tagline->setWordWrap(true);
    welcomeLayout->addWidget(tagline);

    QLabel* templatesLabel = new QLabel(tr("Start a new document"), m_welcomeView);
    templatesLabel->setObjectName(QStringLiteral("VelaWelcomeSection"));
    welcomeLayout->addWidget(templatesLabel);

    QGridLayout* templatesGrid = new QGridLayout();
    templatesGrid->setSpacing(12);
    const QStringList templates = { tr("Blank"), tr("Resume"), tr("Letter"),
                                    tr("Report"), tr("Invoice") };
    for (int i = 0; i < templates.size(); ++i) {
        QPushButton* card = new QPushButton(templates[i], m_welcomeView);
        card->setObjectName(QStringLiteral("VelaTemplateCard"));
        card->setMinimumHeight(160);
        card->setMinimumWidth(140);
        connect(card, &QPushButton::clicked, this, [this, name = templates[i]]() {
            openTemplate(name);
        });
        templatesGrid->addWidget(card, i / 5, i % 5);
    }
    welcomeLayout->addLayout(templatesGrid);

    QLabel* recentLabel = new QLabel(tr("Recent documents"), m_welcomeView);
    recentLabel->setObjectName(QStringLiteral("VelaWelcomeSection"));
    welcomeLayout->addWidget(recentLabel);

    QHBoxLayout* recentRow = new QHBoxLayout();
    recentRow->setSpacing(12);
    const auto recent = m_recentFiles->list();
    if (recent.isEmpty()) {
        QLabel* empty = new QLabel(tr("No recent files — open one to get started."), m_welcomeView);
        empty->setStyleSheet(QStringLiteral("color: #777587; font-style: italic;"));
        recentRow->addWidget(empty);
    } else {
        for (int i = 0; i < qMin(4, recent.size()); ++i) {
            const QString path = recent[i];
            QPushButton* recentBtn = new QPushButton(QFileInfo(path).fileName(), m_welcomeView);
            recentBtn->setObjectName(QStringLiteral("VelaRecentFileCard"));
            recentBtn->setMinimumHeight(80);
            recentBtn->setMinimumWidth(180);
            recentBtn->setToolTip(path);
            connect(recentBtn, &QPushButton::clicked, this, [this, path]() {
                openDocument(path);
            });
            recentRow->addWidget(recentBtn);
        }
    }
    welcomeLayout->addLayout(recentRow);

    welcomeLayout->addStretch(1);
    m_centralStack->addWidget(m_welcomeView);

#ifdef VE_HAS_WEBENGINE
    m_webView = new QWebEngineView(this);
    m_webView->setObjectName(QStringLiteral("VelaEditorCanvas"));
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

    const QUrl url = editorUrlForRuntime();
    VE_LOG_INFO(QStringLiteral("MainWindow: editor URL = %1").arg(url.toString()));
    m_webView->setUrl(url);

    connect(m_bridge, &EditorBridge::statsChanged,
            this, &MainWindow::onEditorStatsChanged);
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
    connect(m_tabBar, &TabBar::closeRequested,
            this, &MainWindow::closeDocument);

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
    if (!m_currentFile.isEmpty())
        title = QFileInfo(m_currentFile).fileName() + QStringLiteral(" — Vela Docs");
    else
        title = tr("Untitled — Vela Docs");
    if (m_controller && m_controller->hasUnsavedChanges())
        title.prepend(QChar(0x2022) + QStringLiteral(" "));
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
        QAction* act = m_menuRecent->addAction(QFileInfo(path).fileName());
        act->setToolTip(path);
        connect(act, &QAction::triggered, this, [this, path]() { openDocument(path); });
    }
    m_menuRecent->addSeparator();
    QAction* clear = m_menuRecent->addAction(tr("Clear List"));
    connect(clear, &QAction::triggered, m_recentFiles, &RecentFiles::clear);
}

void MainWindow::showWelcomeView()
{
    if (m_centralStack) m_centralStack->setCurrentIndex(0);
}

void MainWindow::showEditorView()
{
    if (m_centralStack && m_centralStack->count() > 1)
        m_centralStack->setCurrentIndex(1);
}

void MainWindow::openTemplate(const QString& templateName)
{
    Q_UNUSED(templateName);
    newDocument();
}

bool MainWindow::openDocument(const QString& filePath)
{
    if (!m_controller->openDocument(filePath))
        return false;
    m_currentFile = filePath;
    m_recentFiles->add(filePath);
    showEditorView();
    updateWindowTitle();
    return true;
}

bool MainWindow::saveDocument()
{
    if (!m_controller) return false;
    if (m_currentFile.isEmpty()) return saveDocumentAs();
    return m_controller->saveDocument(m_currentFile);
}

bool MainWindow::saveDocumentAs()
{
    if (!m_controller) return false;
    const QString dest = QFileDialog::getSaveFileName(this,
        tr("Save Document As"),
        QStringLiteral("Untitled.docx"),
        tr("Word Document (*.docx);;All Files (*)"));
    if (dest.isEmpty()) return false;
    if (m_controller->saveDocument(dest)) {
        m_currentFile = dest;
        m_recentFiles->add(dest);
        updateWindowTitle();
        return true;
    }
    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
//  Slots
// ═══════════════════════════════════════════════════════════════════════════

void MainWindow::newDocument()
{
    if (m_controller) m_controller->newDocument();
    m_currentFile.clear();
    showEditorView();
    updateWindowTitle();
}

void MainWindow::openDocumentDialog()
{
    const QStringList filters{
        tr("All Supported (*.docx *.doc *.odt *.rtf *.md *.txt *.html)"),
        tr("Word Document (*.docx *.doc)"),
        tr("OpenDocument Text (*.odt)"),
        tr("Rich Text Format (*.rtf)"),
        tr("Markdown (*.md)"),
        tr("Text File (*.txt)"),
        tr("HTML File (*.html)"),
        tr("All Files (*)")
    };
    const QString path = QFileDialog::getOpenFileName(this,
        tr("Open Document"), QString(), filters.join(QStringLiteral(";;")));
    if (!path.isEmpty()) openDocument(path);
}

void MainWindow::closeDocument()
{
    if (!m_controller) return;
    if (m_controller->hasUnsavedChanges()) {
        const auto ret = QMessageBox::question(this, tr("Unsaved Changes"),
            tr("Save changes to %1?").arg(QFileInfo(m_currentFile).fileName()),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (ret == QMessageBox::Cancel) return;
        if (ret == QMessageBox::Save && !saveDocument()) return;
    }
    m_controller->closeActiveDocument();
    m_currentFile.clear();
    showWelcomeView();
    updateWindowTitle();
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
    const bool visible = !m_focusMode;
    if (m_ribbon)   m_ribbon->setVisible(visible);
    if (m_tabBar)   m_tabBar->setVisible(visible);
    if (m_leftRail) m_leftRail->setVisible(visible);
    if (m_sidebar)  m_sidebar->setVisible(visible);
    m_menuBar->setVisible(visible);
    if (m_statusBar) m_statusBar->setVisible(visible);
}

void MainWindow::toggleDarkMode()
{
    m_darkMode = m_actDarkMode->isChecked();
    setProperty("dark", m_darkMode);
    style()->unpolish(this);
    style()->polish(this);
    if (m_statusBar) m_statusBar->showMessage(
        m_darkMode ? tr("Dark mode on") : tr("Dark mode off"), 2000);
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
    QMessageBox::information(this, tr("Preferences"),
        tr("Preferences dialog — coming in v0.3 (theme, language, spell-check)."));
}

void MainWindow::showAbout()
{
    about();
}

void MainWindow::onActiveDocumentChanged()
{
    updateWindowTitle();
    showEditorView();
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
            openDocument(localPath);
            event->acceptProposedAction();
        }
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_controller && m_controller->hasUnsavedChanges()) {
        const auto ret = QMessageBox::question(this, tr("Unsaved Changes"),
            tr("Save changes before quitting?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (ret == QMessageBox::Cancel) { event->ignore(); return; }
        if (ret == QMessageBox::Save && !saveDocument()) { event->ignore(); return; }
    }
    event->accept();
}

} // namespace vela