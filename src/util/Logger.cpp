// ════════════════════════════════════════════════════════════════════════════
//  Logger.cpp
// ════════════════════════════════════════════════════════════════════════════
#include "util/Logger.h"

#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>

namespace vela {

namespace {
    /// Deliberately not Application::logDirectory(): a low-level utility must
    /// not depend on the app shell, and pulling it in made the unit tests fail
    /// to link. Same path, computed locally.
    QString logDirectory()
    {
        return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
             + QStringLiteral("/logs");
    }

    // The actual message handler installed via qInstallMessageHandler.
    void qtMessageHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg)
    {
        Q_UNUSED(ctx);
        Logger::instance().write(type, msg);
    }
}

Logger& Logger::instance()
{
    static Logger s_instance;
    return s_instance;
}

Logger::Logger(QObject* parent)
    : QObject(parent)
{
    QDir().mkpath(logDirectory());

    const QString fileName = QStringLiteral("vela-%1.log")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd")));

    m_file.setFileName(logDirectory() + QLatin1Char('/') + fileName);
    m_file.open(QIODevice::Append | QIODevice::Text);
}

Logger::~Logger()
{
    if (m_file.isOpen()) {
        m_file.close();
    }
}

void Logger::installMessageHandler()
{
    qInstallMessageHandler(qtMessageHandler);
    qSetMessagePattern(QStringLiteral("[%{time HH:mm:ss.zzz}] [%{type}] %{message}"));
}

void Logger::setVerbose(bool on)
{
    m_verbose = on;
}

void Logger::write(QtMsgType level, const QString& message)
{
    QMutexLocker locker(&m_mutex);

    if (level == QtDebugMsg && !m_verbose) {
        return;
    }

    const QString line = QStringLiteral("[%1] [%2] %3\n")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz")),
             QString::fromLatin1(
                 level == QtDebugMsg    ? "DEBUG" :
                 level == QtInfoMsg     ? "INFO " :
                 level == QtWarningMsg  ? "WARN " :
                 level == QtCriticalMsg ? "ERROR" : "FATAL "),
             message);

    // File output
    if (m_file.isOpen()) {
        QTextStream ts(&m_file);
        ts << line;
        ts.flush();
    }

    // Console output (visible when run from a terminal)
    fprintf(stderr, "%s", qPrintable(line));
    fflush(stderr);
}

} // namespace vela
