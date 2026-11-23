// ════════════════════════════════════════════════════════════════════════════
//  main.cpp — Application entry point for Vela Docs
// ════════════════════════════════════════════════════════════════════════════
//  A native C++/Qt 6 word processor combining MS Word + Google Docs features.
// ════════════════════════════════════════════════════════════════════════════

#include "app/Application.h"
#include "util/Logger.h"

#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFileInfo>
#include <QStandardPaths>
#include <QtGlobal>

#ifdef Q_OS_WIN
#  include <windows.h>
#  include <crtdbg.h>
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    // A normal launch is a GUI process (WIN32_EXECUTABLE). Only reuse a console
    // that already exists — cmd, PowerShell, or Visual Studio. AllocConsole()
    // would pop a black log window on every double-click.
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
#endif

    // High-DPI scaling is automatic in Qt 6 — but allow opting into the
    // policy-based scaling for users with multi-monitor mixed DPI setups.
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    // The Application class is a QApplication subclass; it constructs itself
    // with appropriate org/app name for QSettings storage.
    vela::Application app(argc, argv);

    // Set up logging early so we capture startup issues
    vela::Logger::instance().installMessageHandler();

    VE_LOG_INFO("──────────────────────────────────────────────");
    VE_LOG_INFO("  Vela Docs v" VE_VERSION " starting up");
    VE_LOG_INFO("  Qt " QT_VERSION_STR " | Build ABI: " << QT_POINTER_SIZE << "-bit");
    VE_LOG_INFO("──────────────────────────────────────────────");

    // ── Command line parsing ────────────────────────────────────────────
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Vela Docs — a modern word processor"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("file"),
        QStringLiteral("Optional document file to open (.docx, .doc, .odt, .rtf, .md, .txt, .html)"));

    QCommandLineOption optNoSplash({QStringLiteral("no-splash")},
        QStringLiteral("Skip the splash screen on startup"));
    parser.addOption(optNoSplash);

    QCommandLineOption optSafeMode({QStringLiteral("safe-mode")},
        QStringLiteral("Disable auto-save and revision history (for debugging)"));
    parser.addOption(optSafeMode);

    // NB: addVersionOption() already owns the "v" short flag, so verbose logging
    // takes only the long form.
    QCommandLineOption optVerbose({QStringLiteral("verbose")},
        QStringLiteral("Enable verbose debug logging"));
    parser.addOption(optVerbose);

    parser.process(app);

    if (parser.isSet(optVerbose)) {
        vela::Logger::instance().setVerbose(true);
        VE_LOG_DEBUG("Verbose logging enabled");
    }

    // ── Initial document ────────────────────────────────────────────────
    QString initialFile;
    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) {
        const QString candidate = args.first();
        if (QFileInfo::exists(candidate)) {
            initialFile = QFileInfo(candidate).absoluteFilePath();
            VE_LOG_INFO("Opening initial file: " << initialFile);
        } else {
            VE_LOG_WARNING("File does not exist: " << candidate);
        }
    }

    // ── Show main window ────────────────────────────────────────────────
    app.showMainWindow(initialFile);

    VE_LOG_INFO("Entering Qt event loop.");
    const int exitCode = app.exec();
    VE_LOG_INFO("Exiting with code " << exitCode);
    return exitCode;
}
