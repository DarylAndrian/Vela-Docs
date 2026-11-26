// ════════════════════════════════════════════════════════════════════════════
//  main.cpp — Application entry point for Vela Docs
// ════════════════════════════════════════════════════════════════════════════
//  A native C++/Qt 6 word processor combining MS Word + Google Docs features.
// ════════════════════════════════════════════════════════════════════════════

#include "app/Application.h"
#include "util/Logger.h"

#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QtGlobal>

#ifdef Q_OS_WIN
#  include <QCryptographicHash>
#  include <QDeadlineTimer>
#  include <QJsonArray>
#  include <QJsonDocument>
#  include <QLocalServer>
#  include <QLocalSocket>
#  include <QLockFile>
#  include <QMessageBox>
#  include <windows.h>

namespace {

constexpr int kLaunchTimeoutMs = 5000;
constexpr int kMaxLaunchBytes = 256 * 1024;
enum class LaunchResult { Listening, Forwarded, Failed };

QString documentLaunchName(const QString& dataDirectory)
{
    DWORD sessionId = 0;
    ProcessIdToSessionId(GetCurrentProcessId(), &sessionId);
    const auto key = (dataDirectory + u'|' + QCoreApplication::applicationFilePath()).toCaseFolded()
        + u'|' + QString::number(sessionId);
    const auto digest = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha256);
    return QStringLiteral("VelaDocs-") + QString::fromLatin1(digest.toHex().left(32));
}

void receiveDocumentLaunches(QLocalServer& server, vela::Application& app)
{
    QObject::connect(&server, &QLocalServer::newConnection, &app, [&server, &app] {
        while (auto* socket = server.nextPendingConnection()) {
            socket->setReadBufferSize(kMaxLaunchBytes);
            QObject::connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
            QTimer::singleShot(kLaunchTimeoutMs, socket, &QLocalSocket::abort);
            const auto receive = [socket, &app] {
                if (!socket->canReadLine()) {
                    if (socket->bytesAvailable() >= kMaxLaunchBytes) socket->abort();
                    return;
                }
                const auto message = QJsonDocument::fromJson(socket->readLine(kMaxLaunchBytes + 1));
                if (!message.isArray()) {
                    socket->abort();
                    return;
                }
                QStringList paths;
                for (const auto& value : message.array()) {
                    const QString path = value.toString();
                    if (!QDir::isAbsolutePath(path)) {
                        socket->abort();
                        return;
                    }
                    paths.append(path);
                }
                app.enqueueDocumentLaunch(paths);
                socket->write("1", 1);
                socket->disconnectFromServer();
            };
            QObject::connect(socket, &QLocalSocket::readyRead, socket, receive);
            receive();
        }
    });
}

LaunchResult routeDocumentLaunch(QLockFile& lock, QLocalServer& server, const QString& name,
                                 const QStringList& files)
{
    // On Windows Qt permits more than one listener on a named pipe. Keep the
    // lock for the lifetime of the listener, including while its window starts.
    QDeadlineTimer deadline(kLaunchTimeoutMs);
    do {
        if (lock.tryLock(0)) {
            server.setSocketOptions(QLocalServer::UserAccessOption);
            if (server.listen(name)) return LaunchResult::Listening;
            VE_LOG_WARNING("Could not listen for document launches: " << server.errorString());
            return LaunchResult::Failed;
        }
        if (lock.error() != QLockFile::LockFailedError) return LaunchResult::Failed;

        QLocalSocket socket;
        socket.connectToServer(name);
        if (socket.waitForConnected(100)) {
            DWORD serverPid = 0;
            if (GetNamedPipeServerProcessId(reinterpret_cast<HANDLE>(socket.socketDescriptor()), &serverPid))
                AllowSetForegroundWindow(serverPid);
            const auto request = QJsonDocument(QJsonArray::fromStringList(files)).toJson(QJsonDocument::Compact) + '\n';
            if (request.size() > kMaxLaunchBytes || socket.write(request) != request.size())
                return LaunchResult::Failed;
            while (socket.bytesToWrite() > 0) {
                if (!socket.waitForBytesWritten(static_cast<int>(deadline.remainingTime())))
                    return LaunchResult::Failed;
            }
            if (!socket.bytesAvailable())
                socket.waitForReadyRead(static_cast<int>(deadline.remainingTime()));
            return socket.read(1) == "1" ? LaunchResult::Forwarded : LaunchResult::Failed;
        }
        Sleep(30);
    } while (!deadline.hasExpired());
    return LaunchResult::Failed;
}

} // namespace
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
        QStringLiteral("Word documents (.docx) to open"), QStringLiteral("[file...]"));

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
    QStringList initialFiles;
    for (const QString& candidate : parser.positionalArguments()) {
        if (QFileInfo::exists(candidate)) {
            initialFiles.append(QFileInfo(candidate).absoluteFilePath());
            VE_LOG_INFO("Requested file: " << initialFiles.last());
        } else {
            VE_LOG_WARNING("File does not exist: " << candidate);
        }
    }

#ifdef Q_OS_WIN
    // Explorer can launch us from System32, where Qt WebEngine can crash
    // during initialization. Resolve the document against the caller's
    // directory above before switching to the deployed Qt runtime directory.
    if (!QDir::setCurrent(app.applicationDirPath())) {
        VE_LOG_WARNING("Could not set the working directory to " << app.applicationDirPath());
    }
    const QString localData = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    const QString launchName = documentLaunchName(localData);
    QLockFile launchLock(QDir(localData).filePath(launchName + QStringLiteral(".lock")));
    launchLock.setStaleLockTime(0);
    QLocalServer launchServer;
    if (!parser.isSet(optSafeMode)) {
        const auto result = QDir().mkpath(localData)
            ? routeDocumentLaunch(launchLock, launchServer, launchName, initialFiles)
            : LaunchResult::Failed;
        if (result == LaunchResult::Forwarded) return 0;
        if (result == LaunchResult::Failed) {
            QMessageBox::warning(nullptr, QObject::tr("Vela Docs"),
                QObject::tr("Could not open the Vela Docs window. Close any open Vela Docs window and try again."));
            return 1;
        }
        receiveDocumentLaunches(launchServer, app);
    }
#endif

    // ── Show main window ────────────────────────────────────────────────
    app.enqueueDocumentLaunch(initialFiles);

    VE_LOG_INFO("Entering Qt event loop.");
    const int exitCode = app.exec();
    VE_LOG_INFO("Exiting with code " << exitCode);
    return exitCode;
}
