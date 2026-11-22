// ════════════════════════════════════════════════════════════════════════════
//  RecentFiles.cpp
// ════════════════════════════════════════════════════════════════════════════
#include "app/RecentFiles.h"

#include <QFileInfo>
#include <QSettings>

namespace vela {

RecentFiles::RecentFiles(QObject* parent)
    : QObject(parent)
{
    load();
}

void RecentFiles::add(const QString& path)
{
    if (path.isEmpty()) return;
    const QString abs = QFileInfo(path).absoluteFilePath();
    m_list.removeAll(abs);
    m_list.prepend(abs);
    while (m_list.size() > m_maxSize) {
        m_list.removeLast();
    }
    persist();
    emit changed();
}

void RecentFiles::remove(const QString& path)
{
    if (m_list.removeAll(path) > 0) {
        persist();
        emit changed();
    }
}

void RecentFiles::clear()
{
    if (m_list.isEmpty()) return;
    m_list.clear();
    persist();
    emit changed();
}

void RecentFiles::setMaxSize(int size)
{
    m_maxSize = qMax(1, size);
    while (m_list.size() > m_maxSize) {
        m_list.removeLast();
    }
}

void RecentFiles::persist()
{
    QSettings s;
    s.beginGroup(QStringLiteral("RecentFiles"));
    s.setValue(QStringLiteral("list"), m_list);
    s.setValue(QStringLiteral("maxSize"), m_maxSize);
    s.endGroup();
}

void RecentFiles::load()
{
    QSettings s;
    s.beginGroup(QStringLiteral("RecentFiles"));
    m_list = s.value(QStringLiteral("list"), QStringList()).toStringList();
    m_maxSize = s.value(QStringLiteral("maxSize"), 10).toInt();
    s.endGroup();
}

} // namespace vela
