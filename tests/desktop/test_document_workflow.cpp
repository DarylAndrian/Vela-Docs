#include "app/Application.h"
#include "app/MainWindow.h"
#include "app/TabBar.h"
#include "document/DocumentController.h"
#include "document/TabSession.h"
#include "formats/DocxExporter.h"
#include "formats/DocxImporter.h"
#include "services/DraftRecovery.h"
#include "services/SettingsStore.h"
#include "ui/dialogs/PageSetupDialog.h"
#include "ui/dialogs/SaveChangesDialog.h"
#include "util/Zip.h"

#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QToolButton>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineView>

#include <cstdio>
#include <functional>
#include <windows.h>

namespace {

bool expect(bool result, const char* label)
{
    std::fprintf(result ? stdout : stderr, "%s %s\n", result ? "PASS" : "FAIL", label);
    return result;
}

void waitForEvents(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

QVariant evaluate(QWebEnginePage* page, const QString& source)
{
    QVariant value;
    QEventLoop loop;
    page->runJavaScript(source, [&value, &loop](const QVariant& result) {
        value = result;
        loop.quit();
    });
    loop.exec();
    return value;
}

QJsonObject textDocument(const QString& text)
{
    const QJsonObject run{{"type", "text"}, {"text", text}};
    const QJsonObject paragraph{{"type", "paragraph"}, {"content", QJsonArray{run}}};
    return {{"type", "doc"}, {"content", QJsonArray{paragraph}}};
}

template <typename Dialog>
bool respondToDialog(const std::function<void()>& action, int result)
{
    bool handled = false;
    QTimer monitor;
    QObject::connect(&monitor, &QTimer::timeout, [&] {
        if (auto* dialog = qobject_cast<Dialog*>(QApplication::activeModalWidget())) {
            handled = true;
            dialog->done(result);
        }
    });
    monitor.start(10);
    action();
    return handled;
}

} // namespace

int main(int argc, char** argv)
{
    QTemporaryDir files;
    QStandardPaths::setTestModeEnabled(true);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, files.path());
    vela::Application app(argc, argv);
    vela::SettingsStore().setAutoSaveEnabled(false);
    vela::MainWindow window;
    window.setAttribute(Qt::WA_DontShowOnScreen);
    window.show();

    const QString first = files.filePath("first.docx");
    const QString invalid = files.filePath("invalid.docx");
    const QString wordFile = files.filePath("word-empty-runs.docx");
    QString error;
    vela::DocxExporter exporter;
    if (!expect(exporter.exportToFile(first, textDocument("Initial text"), error), "Create initial DOCX")) return 1;
    if (!expect(vela::Zip::write(invalid, {{"word/document.xml", "<w:document"}}), "Create malformed DOCX")) return 1;
    if (!expect(vela::Zip::write(wordFile, {{"word/document.xml", R"XML(
        <w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
          <w:body>
            <w:p>
              <w:r><w:rPr><w:b/></w:rPr></w:r>
              <w:r><w:t>Word</w:t></w:r>
              <w:r><w:t xml:space="preserve">  </w:t></w:r>
              <w:hyperlink>
                <w:r><w:t/></w:r>
                <w:r><w:t>link</w:t></w:r>
              </w:hyperlink>
            </w:p>
            <w:p><w:r><w:rPr><w:i/></w:rPr></w:r></w:p>
          </w:body>
        </w:document>)XML"}}), "Create Word DOCX with empty runs")) return 1;

    auto* web = window.findChild<QWebEngineView*>("VelaEditorCanvas");
    auto* welcome = window.findChild<QPushButton*>("VelaWelcomeNew");
    auto* tabs = window.findChild<vela::TabBar*>();
    if (!expect(web && welcome && tabs && welcome->isVisible(), "Welcome displays New Document")) return 1;
    auto* page = web->page();
    if (!expect(page->profile()->isSpellCheckEnabled() && page->profile()->spellCheckLanguages().contains("en-US")
                && QFile::exists(QCoreApplication::applicationDirPath() + "/qtwebengine_dictionaries/en-US.bdic"),
                "US English spell-check and the offline dictionary are available")) return 1;
    for (int i = 0; i < 50 && !evaluate(page, "!!window.__velaEditorSnapshot").toBool(); ++i) waitForEvents(100);
    if (!expect(evaluate(page, "!!window.__velaEditorSnapshot").toBool(), "Native editor connects")) return 1;
    welcome->click();
    waitForEvents(150);
    if (!expect(evaluate(page, "window.__editor.getText() === '' && window.__editor.isEditable").toBool(), "New opens a blank, editable document")) return 1;
    if (!expect(!tabs->isVisible(), "One document does not need a tab strip")) return 1;
    if (!expect(window.openDocument(wordFile), "Open DOCX with empty Word runs")) return 1;
    if (!expect(tabs->isVisible() && tabs->count() == 2, "Two documents show the tab strip")) return 1;
    if (!expect(evaluate(page, "window.__editor.state.doc.firstChild.textContent === 'Word  link' && "
                              "window.__editor.state.doc.lastChild.type.name === 'paragraph' && "
                              "window.__editor.state.doc.lastChild.childCount === 0").toBool(),
                "Empty text runs preserve document text, spaces and blank paragraphs")) return 1;
    auto* themeButton = window.findChild<QToolButton*>("VelaThemeButton");
    if (!expect(themeButton != nullptr, "Find the theme button")) return 1;
    const QString paperColors = R"JS((() => {
        const ink = getComputedStyle(window.__editor.view.dom).color;
        const paper = getComputedStyle(document.querySelector('.page')).backgroundColor;
        return ink + ' on ' + paper;
    })())JS";
    const QVariant lightPaper = evaluate(page, paperColors);
    themeButton->click();
    if (!expect(evaluate(page, "document.documentElement.dataset.theme === 'dark'").toBool()
                && evaluate(page, paperColors) == lightPaper,
                "Dark mode keeps document text legible on white paper")) return 1;
    themeButton->click();
    window.closeDocument();
    if (!expect(!tabs->isVisible() && tabs->count() == 1, "Closing the second document hides the tab strip")) return 1;

    const QString longFile = files.filePath("long.docx");
    auto longBody = textDocument("A paragraph in a long document.");
    const auto paragraph = longBody.value("content").toArray().first();
    QJsonArray paragraphs;
    for (int i = 0; i < 100; ++i) paragraphs.append(paragraph);
    longBody.insert("content", paragraphs);
    if (!expect(exporter.exportToFile(longFile, longBody, error) && window.openDocument(longFile),
                "Open a long DOCX")) return 1;
    if (!expect(evaluate(page, R"JS((() => {
        const canvas = document.querySelector('.canvas');
        const paper = document.querySelector('.page');
        const last = window.__editor.view.dom.lastElementChild;
        const viewport = canvas.getBoundingClientRect();
        canvas.scrollTop = 0;
        if (last.getBoundingClientRect().bottom <= viewport.bottom) return false;
        canvas.scrollTop = canvas.scrollHeight;
        const end = last.getBoundingClientRect();
        return canvas.scrollTop > 0 && paper.scrollTop === 0
            && end.top >= viewport.top && end.bottom <= viewport.bottom;
    })())JS").toBool(), "The canvas scrollbar reaches the last paragraph")) return 1;
    window.closeDocument();

    if (!expect(window.openDocument(first), "Open initial document")) return 1;
    waitForEvents(150);
    auto* original = tabs->currentSession();
    auto* controller = original->controller();

    QElapsedTimer elapsed;
    elapsed.start();
    evaluate(page, "window.__editor.commands.insertContent('Latest edit ')");
    if (!expect(window.saveDocument() && elapsed.elapsed() < 400, "Save before the editor debounce")) return 1;
    QJsonObject saved;
    vela::DocxImporter importer;
    if (!expect(importer.importFromFile(first, saved, error) && QJsonDocument(saved).toJson().contains("Latest edit"), "Latest edit is on disk")) return 1;

    const HANDLE locked = CreateFileW(reinterpret_cast<LPCWSTR>(first.utf16()), GENERIC_READ,
                                     FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (!expect(locked != INVALID_HANDLE_VALUE, "Lock the existing document against replacement")) return 1;
    const bool replaced = exporter.exportToFile(first, textDocument("Replacement must fail"), error);
    CloseHandle(locked);
    if (!expect(!replaced && importer.importFromFile(first, saved, error)
                && QJsonDocument(saved).toJson().contains("Latest edit"), "Failed replacement preserves the original")) return 1;

    evaluate(page, "window.__editor.commands.insertContent('Unsaved draft ')");
    const auto activeId = controller->activeDocument()->id();
    window.newDocument();
    waitForEvents(150);
    if (!expect(tabs->currentSession() != original && original->isDirty()
                && evaluate(page, "window.__editor.getText() === '' && !window.__editor.can().undo()").toBool(),
                "New tab keeps the earlier unsaved edits and starts with independent history")) return 1;
    tabs->setCurrentIndex(tabs->indexOfSession(original));
    waitForEvents(150);
    if (!expect(evaluate(page, "window.__editor.getText().includes('Unsaved draft') && window.__editor.can().undo()").toBool(),
                "Switching back restores the draft and undo history")) return 1;
    if (!expect(respondToDialog<vela::SaveChangesDialog>([&] { window.closeDocument(); }, vela::SaveChangesDialog::Cancel), "Close prompts for pending edits")) return 1;
    if (!expect(tabs->currentSession() == original, "Cancel preserves the active tab")) return 1;

    bool opened = true;
    QTimer monitor;
    QObject::connect(&monitor, &QTimer::timeout, [&] {
        auto* modal = QApplication::activeModalWidget();
        if (auto* warning = qobject_cast<QMessageBox*>(modal)) warning->accept();
    });
    monitor.start(10);
    opened = window.openDocument(invalid);
    monitor.stop();
    if (!expect(!opened && tabs->currentSession() == original && controller->activeDocument()->id() == activeId
                && controller->hasUnsavedChanges(), "Malformed Open preserves the active draft")) return 1;

    evaluate(page, "window.__velaSetPageSettings({size:'Legal', orientation:'landscape', headerText:'Audit Header', footerText:'Page {PAGE}'})");
    if (!expect(window.saveDocument(), "Save per-document layout")) return 1;
    QHash<QString, QByteArray> archive;
    if (!expect(vela::Zip::openForReading(first, archive) && archive.value("word/header1.xml").contains("Audit Header")
                && archive.value("word/footer1.xml").contains("PAGE")
                && archive.value("word/document.xml").contains("w:w=\"20160\""), "DOCX contains the header, footer field and Legal size")) return 1;
    window.closeDocument();
    waitForEvents(150);
    if (!expect(evaluate(page, "window.__velaGetPageSettings().orientation === 'portrait'").toBool(), "New document starts with its own layout")) return 1;
    if (!expect(window.openDocument(first), "Reopen saved document")) return 1;
    waitForEvents(150);
    if (!expect(evaluate(page, "window.__velaGetPageSettings().orientation === 'landscape'").toBool(), "Reopen restores layout")) return 1;

    evaluate(page, "window.__exec('openInsertImage')");
    if (!expect(evaluate(page, "!!document.querySelector('.modal-overlay')").toBool(), "Insert Image opens")) return 1;
    if (!expect(evaluate(page, "document.querySelector('.modal-dialog [data-action=cancel]').click(); !document.querySelector('.modal-overlay')").toBool(), "Insert Image Cancel dismisses overlay")) return 1;
    if (!expect(respondToDialog<vela::PageSetupDialog>([&] {
            evaluate(page, "window.__exec('openPageSetup')");
            waitForEvents(200);
        }, QDialog::Rejected), "Ribbon Margins opens the native Page Setup dialog")) return 1;

    const QString pdfPath = files.filePath("layout.pdf");
    window.exportDocumentToPdf(pdfPath);
    for (int i = 0; i < 50 && !QFile::exists(pdfPath); ++i) waitForEvents(100);
    QFile pdf(pdfPath);
    if (!expect(pdf.open(QIODevice::ReadOnly) && pdf.read(5) == "%PDF-", "Export the document as a PDF")) return 1;
    if (!expect(pdf.readAll().contains("/MediaBox [0 0 1008 612]"), "PDF keeps the Legal landscape page size")) return 1;

    window.newDocument();
    waitForEvents(100);
    evaluate(page, "window.__editor.commands.insertContent('<h2>Keyboard and outline</h2>'); window.__velaToggleOutline(true)");
    if (!expect(evaluate(page, "document.querySelector('.vela-outline button')?.textContent === 'Keyboard and outline'").toBool(),
                "Document outline lists headings from the active tab")) return 1;
    evaluate(page, "window.__exec('openInsertTable')");
    if (!expect(evaluate(page, "document.activeElement?.getAttribute('aria-label') === '3 rows by 3 columns'").toBool(),
                "Table picker presents a keyboard focus target")) return 1;
    if (!expect(evaluate(page, R"JS((() => {
            document.activeElement.dispatchEvent(new KeyboardEvent('keydown', {key:'ArrowRight', bubbles:true}));
            const picked = document.activeElement.getAttribute('aria-label') === '3 rows by 4 columns';
            document.activeElement.click();
            return picked && document.querySelector('.editor table tr')?.children.length === 4;
        })())JS").toBool(), "Arrow keys select a table size and the editor inserts those dimensions")) return 1;

    const QString recoveryRoot = files.filePath("recovery");
    {
        vela::DraftRecovery writer(recoveryRoot);
        vela::Document draft;
        draft.setFilePath(first);
        draft.setBody(textDocument("Recovered content"));
        if (!expect(writer.store(draft), "Persist a recovery copy")) return 1;
        vela::DraftRecovery otherInstance(recoveryRoot);
        if (!expect(otherInstance.findOrphaned().empty(), "A second instance skips active drafts")) return 1;
    }
    vela::DraftRecovery recovery(recoveryRoot);
    const auto drafts = recovery.findOrphaned();
    if (!expect(drafts.size() == 1 && drafts.front().originalPath == first
                && QJsonDocument(drafts.front().body).toJson().contains("Recovered content"), "Read orphaned recovery copy")) return 1;
    return 0;
}
