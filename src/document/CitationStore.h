// CitationStore.h — v0.3.3 (IMP-06)
#pragma once
#include <QString>
#include <QVector>

namespace vela {

/**
 * @brief A CSL JSON source.
 *
 * Lightweight in v0.3.3 — full CSL 1.0.2 schema support lands in v0.3.5.
 * Just enough to insert in-text citations and emit a bibliography section.
 */
struct Citation
{
    QString id;                 ///< e.g. "smith2023"
    QString type = QStringLiteral("article-journal");
    QString title;
    QVector<QString> authors;
    QString issued;             ///< ISO date or year
    QString container;          ///< journal/book title
    QString volume;
    QString page;
    QString doi;
};

class CitationStore
{
public:
    /// All known sources.
    const QVector<Citation>& sources() const { return m_sources; }
    /// Add or update by id.
    void upsert(const Citation& c);
    /// Lookup by id; nullptr if missing.
    const Citation* find(const QString& id) const;
    /// Remove by id.
    bool erase(const QString& id);

    /// Render the bibliography section for a given style ("apa", "mla", ...).
    QString bibliography(const QString& styleId) const;

private:
    QVector<Citation> m_sources;
};

} // namespace vela
