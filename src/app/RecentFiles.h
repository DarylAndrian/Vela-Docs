// ════════════════════════════════════════════════════════════════════════════
//  RecentFiles.h — Tracks most-recently-opened documents
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class QSettings;

namespace vela {

/**
 * @brief Manages the recent-files list (persisted via QSettings).
 *
 * Most-recently-used order. The list is bounded (default 10) and survives
 * app restarts.
 */
class RecentFiles : public QObject
{
    Q_OBJECT
public:
    explicit RecentFiles(QObject* parent = nullptr);

    /// Append a path to the list (moves it to the front if already present).
    void add(const QString& path);

    /// Return paths in MRU order.
    QStringList list() const { return m_list; }

    /// Remove a path from the list.
    void remove(const QString& path);

    /// Empty the list.
    void clear();

    /// Set the maximum size of the list.
    void setMaxSize(int size);

signals:
    void changed();

private:
    void persist();
    void load();

    QStringList m_list;
    int         m_maxSize = 10;
};

} // namespace vela
