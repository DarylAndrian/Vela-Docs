// ════════════════════════════════════════════════════════════════════════════
//  Application.cpp — Loads the "Vela Precision" design system stylesheet
// ════════════════════════════════════════════════════════════════════════════
#include "app/Application.h"
#include "app/MainWindow.h"
#include "services/SettingsStore.h"
#include "services/RevisionStore.h"
#include "util/Logger.h"

#include <QDir>
#include <QAccessibilityHints>
#include <QFile>
#include <QFontDatabase>
#include <QIcon>
#include <QScopedValueRollback>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QStyleHints>
#include <QSurfaceFormat>

namespace vela {

Application::Application(int& argc, char** argv)
    : QApplication(argc, argv)
{
    setOrganizationName(QStringLiteral("Vela Docs"));
    setOrganizationDomain(QStringLiteral("veladocs.app"));
    setApplicationName(QStringLiteral("Vela Docs"));
    setApplicationDisplayName(QStringLiteral("Vela Docs"));
    setApplicationVersion(QStringLiteral(VE_VERSION));
    setWindowIcon(QIcon(QStringLiteral(":/icons/vela-docs.png")));

    // OpenGL profile for WebEngine (no-op if WebEngine isn't built)
    QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setVersion(4, 6);
    fmt.setSamples(4);
    QSurfaceFormat::setDefaultFormat(fmt);

    QDir().mkpath(dataDirectory());
    QDir().mkpath(logDirectory());

    VE_LOG_DEBUG("Data dir:  " << dataDirectory());
    VE_LOG_DEBUG("Log dir:   " << logDirectory());

    m_settings = new SettingsStore(this);
    styleHints()->setColorScheme(m_settings->theme() == SettingsStore::Theme::Dark
        ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);
    registerQtResources();
    loadApplicationStyle();
    connect(styleHints()->accessibility(), &QAccessibilityHints::contrastPreferenceChanged,
            this, &Application::loadApplicationStyle);

    m_revisions = new RevisionStore(dataDirectory() + QStringLiteral("/revisions.db"), this);

    m_launchTimer.setInterval(50);
    connect(&m_launchTimer, &QTimer::timeout, this, &Application::processDocumentLaunch);
    setQuitOnLastWindowClosed(false);
    connect(this, &QApplication::lastWindowClosed, this, [this] {
        if (m_pendingLaunches.isEmpty()) quit();
    });

    VE_LOG_DEBUG("Application constructed");
}

Application::~Application()
{
    VE_LOG_DEBUG("Application destructing");
}

void Application::registerQtResources()
{
    // Phase 6 bundles Material Symbols font; for now we use built-in icons.
}

void Application::loadApplicationStyle()
{
    if (styleHints()->accessibility()->contrastPreference() == Qt::ContrastPreference::HighContrast) {
        setStyleSheet({});
        return;
    }
    // Use Fusion as base for consistent native-feeling rendering
    if (QStyleFactory::keys().contains(QStringLiteral("Fusion"))) {
        setStyle(QStringLiteral("Fusion"));
    }

    // Load the Vela Precision stylesheet from Qt resources
    QFile qss(QStringLiteral(":/styles/vela-precision.qss"));
    if (qss.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString contents = QString::fromUtf8(qss.readAll());
        qss.close();
        setStyleSheet(contents);
        VE_LOG_DEBUG("Loaded Vela Precision QSS ("
                     << contents.size() << " chars)");
    } else {
        VE_LOG_WARNING("Could not load :/styles/vela-precision.qss");
        setStyleSheet(QStringLiteral(R"QSS(
            * { font-family: "Inter", "Segoe UI", system-ui, sans-serif; }
            QMainWindow, QWidget#VelaWelcome { background-color: #f9f9ff; }
            QWidget#VelaEditorArea { background-color: #f5f5f7; }
        )QSS"));
    }

    // ── Fonts ──
    QFont uiFont = pickFont(QStringLiteral("Inter"),
                             { QStringLiteral("Segoe UI Variable Text"),
                               QStringLiteral("Segoe UI") });
    uiFont.setPointSize(10);
    setFont(uiFont);

    VE_LOG_INFO("UI font: " << uiFont.family());

    // Mono font
    QFont monoFont = pickFont(QStringLiteral("JetBrains Mono"),
                               { QStringLiteral("Cascadia Code"),
                                 QStringLiteral("Consolas") });
    monoFont.setStyleHint(QFont::Monospace);
}

QFont Application::pickFont(const QString& preferred,
                              const QStringList& fallbacks) const
{
    QFont f(preferred);
    if (f.exactMatch()) return f;
    for (const QString& fb : fallbacks) {
        QFont candidate(fb);
        if (candidate.exactMatch()) return candidate;
    }
    return f; // return preferred even if missing (Qt will substitute)
}

void Application::showMainWindow(const QString& initialFile)
{
    if (!m_mainWindow) {
        m_mainWindow = new MainWindow();
        m_mainWindow->setAttribute(Qt::WA_DeleteOnClose, true);
    }
    if (m_mainWindow->isMinimized())
        m_mainWindow->setWindowState(m_mainWindow->windowState() & ~Qt::WindowMinimized);
    m_mainWindow->show();
    m_mainWindow->raise();
    m_mainWindow->activateWindow();

    if (!initialFile.isEmpty())
        m_mainWindow->openDocument(initialFile);
}

void Application::enqueueDocumentLaunch(const QStringList& files)
{
    if (files.isEmpty())
        m_pendingLaunches.append(QString());
    else
        m_pendingLaunches.append(files);
    if (!m_launchTimer.isActive()) m_launchTimer.start();
}

void Application::processDocumentLaunch()
{
    // WebEngine synchronization and modal dialogs run nested event loops.
    if (m_handlingLaunch || activeModalWidget()
        || (m_mainWindow && !m_mainWindow->canOpenFromShell()))
        return;

    const QScopedValueRollback handling(m_handlingLaunch, true);
    const QString path = m_pendingLaunches.takeFirst();
    if (m_pendingLaunches.isEmpty()) m_launchTimer.stop();
    showMainWindow(path);
}

QString Application::dataDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString Application::logDirectory()
{
    return dataDirectory() + QStringLiteral("/logs");
}

} // namespace vela
