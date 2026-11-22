// ════════════════════════════════════════════════════════════════════════════
//  PathUtils.cpp
// ════════════════════════════════════════════════════════════════════════════
#include "util/PathUtils.h"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

namespace vela {

QString PathUtils::dataDir()
{
    // Not Application::dataDirectory() — a util must not depend on the app
    // shell; doing so made the unit tests fail to link. Same value.
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString PathUtils::dictionariesDir()
{
    const QString dir = dataDir() + QStringLiteral("/dictionaries");
    QDir().mkpath(dir);
    return dir;
}

QString PathUtils::autosaveDir()
{
    const QString dir = dataDir() + QStringLiteral("/autosave");
    QDir().mkpath(dir);
    return dir;
}

QString PathUtils::crashDumpsDir()
{
    const QString dir = dataDir() + QStringLiteral("/crashdumps");
    QDir().mkpath(dir);
    return dir;
}

QString PathUtils::libreOfficeHelperPath()
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/libreoffice/program/soffice.exe");
}

QString PathUtils::sanitizeFilename(const QString& name)
{
    QString s = name;
    static const QString bad = QStringLiteral("\\/:*?\"<>|\r\n\t");
    for (const QChar& c : bad) {
        s.replace(c, QLatin1Char('_'));
    }
    return s;
}

bool PathUtils::hasExtension(const QString& path, const QString& ext)
{
    return path.endsWith(ext, Qt::CaseInsensitive);
}

} // namespace vela
