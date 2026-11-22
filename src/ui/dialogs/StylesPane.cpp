// ════════════════════════════════════════════════════════════════════════════
//  StylesPane.cpp
// ════════════════════════════════════════════════════════════════════════════
#include "ui/dialogs/StylesPane.h"
#include "document/Document.h"
#include "document/StyleSheet.h"

#include <QFontDatabase>

namespace vela {

StylesPane::StylesPane(QWidget* parent)
    : QListWidget(parent)
    , m_styles(std::make_unique<StyleSheet>())
{
    setObjectName(QStringLiteral("VelaStylesPane"));
    setUniformItemSizes(true);
    setEditTriggers(NoEditTriggers);
    setSelectionMode(SingleSelection);
    setContextMenuPolicy(Qt::DefaultContextMenu);
    connect(this, &QListWidget::itemActivated, this, &StylesPane::onItemActivated);
}

StylesPane::~StylesPane() = default;

void StylesPane::bindDocument(Document* doc)
{
    m_doc = doc;
    refresh();
}

void StylesPane::refresh()
{
    clear();
    for (const Style& s : m_styles->styles()) {
        auto* item = new QListWidgetItem(this);
        QFont f = s.font;
        item->setText(s.name);
        item->setFont(f);
        item->setData(Qt::UserRole, s.aliasId);
        addItem(item);
    }
}

void StylesPane::onItemActivated(QListWidgetItem* item)
{
    if (!item) return;
    const QString aliasId = item->data(Qt::UserRole).toString();
    emit styleActivated(aliasId);
}

} // namespace vela
