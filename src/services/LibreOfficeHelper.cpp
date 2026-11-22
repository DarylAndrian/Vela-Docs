#include "services/LibreOfficeHelper.h"
#include "util/Logger.h"
#include "util/PathUtils.h"

#include <QFileInfo>

namespace vela {

LibreOfficeHelper::LibreOfficeHelper(QObject* parent)
    : QObject(parent)
    , m_proc(new QProcess(this))
{
}

bool LibreOfficeHelper::isInstalled() const
{
    return QFileInfo::exists(PathUtils::libreOfficeHelperPath());
}

QString LibreOfficeHelper::installPath() const
{
    return PathUtils::libreOfficeHelperPath();
}

void LibreOfficeHelper::convert(const QString& from, const QString& to, const QString& targetFormat)
{
    Q_UNUSED(from);
    Q_UNUSED(to);
    Q_UNUSED(targetFormat);
    VE_LOG_DEBUG("LibreOfficeHelper::convert — Phase 4 implementation");
    emit conversionFinished(false, QStringLiteral("Phase 4 not implemented yet"));
}

} // namespace vela
