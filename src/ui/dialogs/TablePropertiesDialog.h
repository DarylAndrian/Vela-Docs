// ════════════════════════════════════════════════════════════════════════════
//  TablePropertiesDialog.h — Table borders, padding, width (v0.3.2 — MAJOR-05)
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QDialog>

namespace vela {

struct TableOptions
{
    int     widthPct           = 100;
    QString align              = QStringLiteral("left"); // left/center/right
    int     cellPadding        = 6;        // px
    int     cellSpacing        = 0;        // px
    QString borderStyle        = QStringLiteral("single"); // single/double/dotted/dashed
    QString borderColor        = QStringLiteral("#888");
    int     borderWidth        = 1;        // px
    QString shadingColor       = QString();
    QString autofit            = QStringLiteral("window"); // window/contents/fixed
};

class TablePropertiesDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TablePropertiesDialog(const TableOptions& initial, QWidget* parent = nullptr);
    TableOptions value() const { return m_value; }

private:
    TableOptions m_value;
};

} // namespace vela
