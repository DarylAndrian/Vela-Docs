// MailMerge.h — Mail merge (v0.3.5 — POLISH-04)
#pragma once
#include <QString>
#include <QStringList>
#include <QVector>

namespace vela {

struct MergeField
{
    QString name;
    QString placeholder;
};

/**
 * @brief Lightweight mail-merge engine.
 *
 * Reads a tabular data source (CSV for v0.3.5; XLSX + SQLite via v0.3.5
 * `xlnt`); for each row, creates a new Document cloned from a template
 * with `{{field}}` placeholders substituted.
 */
class MailMerge
{
public:
    /// Load a CSV data source (header in row 0).
    bool loadCsv(const QString& path);
    /// All rows after the header.
    int rowCount() const { return m_rows.size(); }
    /// Field names (header).
    const QStringList& fields() const { return m_fields; }

    /// Apply the merge: for each row, return the rendered document text.
    QVector<QString> render(const QString& templateText) const;

private:
    QStringList       m_fields;
    QVector<QStringList> m_rows;
};

} // namespace vela
