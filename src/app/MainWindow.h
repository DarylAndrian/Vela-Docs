// ════════════════════════════════════════════════════════════════════════════
//  MainWindow.h — Top-level window for Vela Docs (Stitch "Vela Precision")
//  v0.4.0: adds Backstage widget (Microsoft Word-style Files tab)
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QMainWindow>
#include <QString>
#include <QPointer>
#include <QTemporaryDir>

// QPointer<T> needs T to be complete, so these two cannot stay forward-declared.
#include "document/DocumentController.h"
#include "app/EditorBridge.h"
#include "services/DraftRecovery.h"

class QAction;
class QMenu;
class QMenuBar;
class QPageLayout;
class QToolBar;
class QStatusBar;
class QLabel;
class QPushButton;
class QStackedWidget;
class QWebEngineView;
class QWebChannel;

namespace vela {

class Ribbon;
class TabBar;
class TabSession;
class Sidebar;
class StatusBar;
class DocumentController;
class Document;
class RecentFiles;
class EditorBridge;
class LeftRail;
class WelcomeView;
class Backstage;
class BackstagePanel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    bool openDocument(const QString& filePath);
    bool canOpenFromShell() const;
    bool saveDocument();
    bool saveDocumentAs();
    void exportDocumentToPdf(const QString& path, bool preview = false);

public slots:
    void newDocument();
    void openDocumentDialog();
    void closeDocument();
    void undo();
    void redo();
    void findReplace();
    void toggleFocusMode();
    void toggleDarkMode();
    void zoomIn();
    void zoomOut();
    void zoomReset();
    void about();
    void showPreferences();
    void showAbout();

    // NOTE: the v0.4.0 "Files tab" / Backstage slots used to be declared here
    // (toggleBackstage, openBackstagePanel, closeBackstage, onBackstage*).
    // They were never implemented and app/Backstage.cpp is not in the build, so
    // moc emitted calls to symbols that did not exist and the target failed to
    // link. Reintroduce them together with a repaired Backstage module.

protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void onDirtyStateChanged(bool dirty);
    void onLeftRailViewActivated(int view);
    void onLeftRailViewDeactivated(int view);
    void onRibbonCommand(const QString& command);
    void onEditorStatsChanged(int words, int characters, int pages);

private:
    friend class EditorBridge;
    void buildActions();
    void buildMenuBar();
    void buildTopBar();
    void buildTabStrip();
    void buildLeftRail();
    void buildRibbon();
    void buildSidebar();
    void buildStatusBar();
    void buildEditorArea();
    void buildBackstage();          // v0.4.0 — new
    void wireSignals();
    bool synchronizeEditor(QJsonObject* pageSettings = nullptr, bool reportFailure = true);
    bool confirmDocumentClose(TabSession* session, const QString& prompt);
    TabSession* createSession();
    void addSession(TabSession* session);
    void activateSession(TabSession* session);
    void closeSession(TabSession* session);
    QString currentFile() const;
    bool saveDocumentTo(const QString& path);
    void openRecentDocument(const QString& path);
    void backUpDraft();
    void offerDraftRecovery();
    void clearBackup(TabSession* session);
    bool preparePrintLayout(QPageLayout& layout);
    void printDocument();
    void updateWindowTitle();
    void updateRecentFilesMenu();
    void showWelcomeView();
    void showEditorView();
    void updateWorkspaceChrome();
    void updateEditorAppearance();
    void setupWebChannel();

    QPointer<DocumentController> m_controller;
    TabSession*                  m_session = nullptr;
    QPointer<EditorBridge>       m_bridge;
    RecentFiles*                 m_recentFiles = nullptr;

    // ── UI components ──
    QMenuBar*        m_menuBar     = nullptr;
    QWidget*         m_topBar      = nullptr;
    QLabel*          m_documentStatus = nullptr;
    TabBar*          m_tabBar      = nullptr;
    LeftRail*        m_leftRail    = nullptr;
    Ribbon*          m_ribbon      = nullptr;
    Sidebar*         m_sidebar     = nullptr;
    StatusBar*       m_statusBar   = nullptr;
    QStackedWidget*  m_centralStack = nullptr;
    WelcomeView*     m_welcomeView  = nullptr;

    // ── Editor surface ──
    QWebEngineView*  m_webView     = nullptr;
    QWebChannel*     m_webChannel  = nullptr;

    // ── v0.4.0 Backstage ──
    Backstage*       m_backstage   = nullptr;
    BackstagePanel*  m_backstageSavePanel       = nullptr;
    BackstagePanel*  m_backstageSaveAsPanel     = nullptr;
    BackstagePanel*  m_backstageOpenPanel       = nullptr;
    BackstagePanel*  m_backstagePrintPanel      = nullptr;
    BackstagePanel*  m_backstagePrintPreviewPanel = nullptr;
    BackstagePanel*  m_backstageOptionsPanel    = nullptr;
    BackstagePanel*  m_backstageHelpPanel       = nullptr;
    BackstagePanel*  m_backstageRecentPanel     = nullptr;

    // ── File / Edit ──
    QAction* m_actNew         = nullptr;
    QAction* m_actOpen        = nullptr;
    QAction* m_actSave        = nullptr;
    QAction* m_actSaveAs      = nullptr;
    QAction* m_actClose       = nullptr;
    QAction* m_actQuit        = nullptr;
    QAction* m_actUndo        = nullptr;
    QAction* m_actRedo        = nullptr;
    QAction* m_actCut         = nullptr;
    QAction* m_actCopy        = nullptr;
    QAction* m_actPaste       = nullptr;
    QAction* m_actFind        = nullptr;

    // ── Insert ──
    QAction* m_actInsertImage          = nullptr;
    QAction* m_actInsertTable         = nullptr;
    QAction* m_actInsertLink           = nullptr;
    QAction* m_actInsertPageBreak      = nullptr;
    QAction* m_actInsertHorizontalRule = nullptr;

    // ── Format ──
    QAction* m_actBold              = nullptr;
    QAction* m_actItalic            = nullptr;
    QAction* m_actUnderline         = nullptr;
    QAction* m_actStrike            = nullptr;
    QAction* m_actClearFormatting   = nullptr;
    QAction* m_actBulletList        = nullptr;
    QAction* m_actOrderedList       = nullptr;
    QAction* m_actTaskList          = nullptr;
    QAction* m_actParagraph         = nullptr;
    QAction* m_actNormalText        = nullptr;
    QAction* m_actHeading1          = nullptr;
    QAction* m_actHeading2          = nullptr;
    QAction* m_actHeading3          = nullptr;
    QAction* m_actAlignLeft         = nullptr;
    QAction* m_actAlignCenter       = nullptr;
    QAction* m_actAlignRight        = nullptr;
    QAction* m_actAlignJustify      = nullptr;
    QAction* m_actIndent            = nullptr;
    QAction* m_actOutdent           = nullptr;
    // v0.5.0 — page layout / header & footer
    QAction* m_actPageSetup         = nullptr;
    QAction* m_actHeaderFooter      = nullptr;
    QAction* m_actPageNumbering     = nullptr;
    QAction* m_actOrientationPortrait  = nullptr;
    QAction* m_actOrientationLandscape = nullptr;
    QAction* m_actPageModePageless  = nullptr;
    QAction* m_actPageModePaging    = nullptr;

    // ── Table ──
    QAction* m_actTblInsertRowAbove = nullptr;
    QAction* m_actTblInsertRowBelow = nullptr;
    QAction* m_actTblDeleteRow      = nullptr;
    QAction* m_actTblInsertColLeft  = nullptr;
    QAction* m_actTblInsertColRight = nullptr;
    QAction* m_actTblDeleteCol      = nullptr;
    QAction* m_actTblMergeCells     = nullptr;
    QAction* m_actTblSplitCell      = nullptr;
    QAction* m_actTblToggleHeader   = nullptr;
    QAction* m_actTblDeleteTable    = nullptr;

    // ── View / Output / Tools / Help ──
    QAction* m_actFocusMode     = nullptr;
    QAction* m_actOutline       = nullptr;
    QAction* m_actDarkMode      = nullptr;
    QAction* m_actZoomIn        = nullptr;
    QAction* m_actZoomOut       = nullptr;
    QAction* m_actZoomReset     = nullptr;
    QAction* m_actPrint         = nullptr;
    QAction* m_actPrintPreview  = nullptr;
    QAction* m_actExportPdf     = nullptr;
    QAction* m_actPreferences   = nullptr;
    QAction* m_actWordCount     = nullptr;
    QAction* m_actAbout         = nullptr;

    // ── v0.4.0 Files tab ──
    QAction* m_actFiles         = nullptr;  // toggles Backstage
    QAction* m_actToggleTheme   = nullptr;

    QMenu*   m_menuRecent       = nullptr;

    bool    m_focusMode = false;
    bool    m_darkMode  = false;
    bool    m_waitingForEditor = false;
    bool    m_closingWindow = false;
    bool    m_saving = false;
    bool    m_printing = false;
    DraftRecovery m_draftRecovery;
    QHash<QString, QString> m_recoveredNames;
    QTemporaryDir m_printPreviews;
    int m_previewNumber = 0;
};

} // namespace vela
