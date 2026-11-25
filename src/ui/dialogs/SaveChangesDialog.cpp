#include "ui/dialogs/SaveChangesDialog.h"

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

namespace vela {

SaveChangesDialog::SaveChangesDialog(QWidget* parent, const QString& heading,
                                     const QString& documentName)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("VelaSaveChangesDialog"));
    setWindowTitle(tr("Unsaved Changes"));
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->setSizeConstraint(QLayout::SetFixedSize);

    auto* panel = new QWidget(this);
    panel->setMinimumWidth(444);
    root->addWidget(panel);

    auto* content = new QHBoxLayout(panel);
    content->setContentsMargins(24, 24, 24, 24);
    content->setSpacing(14);

    auto* badge = new QLabel(panel);
    badge->setObjectName(QStringLiteral("VelaSaveChangesIcon"));
    badge->setFixedSize(42, 42);
    badge->setAlignment(Qt::AlignCenter);
    content->addWidget(badge, 0, Qt::AlignTop);
    badge->ensurePolished();
    const QIcon icon(QStringLiteral(":/icons/material/edit_document.svg"));
    auto iconPixmap = icon.pixmap(QSize(22, 22), devicePixelRatioF());
    if (!iconPixmap.isNull()) {
        QPainter painter(&iconPixmap);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        const QRect iconBounds(QPoint(), iconPixmap.deviceIndependentSize().toSize());
        painter.fillRect(iconBounds, badge->palette().color(QPalette::WindowText));
        painter.end();
        badge->setPixmap(iconPixmap);
    }

    auto* copy = new QVBoxLayout;
    copy->setSpacing(6);
    content->addLayout(copy, 1);

    auto* title = new QLabel(heading, panel);
    title->setObjectName(QStringLiteral("VelaSaveChangesHeading"));
    title->setTextFormat(Qt::PlainText);
    title->setWordWrap(true);
    copy->addWidget(title);

    auto* description = new QLabel(panel);
    description->setObjectName(QStringLiteral("VelaSaveChangesDescription"));
    description->setTextFormat(Qt::PlainText);
    description->setWordWrap(true);
    description->ensurePolished();
    const QString name = documentName.isEmpty() ? tr("Untitled") : documentName;
    const QString displayedName = description->fontMetrics().elidedText(name, Qt::ElideMiddle, 260);
    const QString message = tr("If you discard, your edits to “%1” will be lost.");
    setAccessibleDescription(message.arg(name));
    description->setText(message.arg(displayedName));
    if (displayedName != name) {
        description->setToolTip(QStringLiteral("<span>%1</span>").arg(name.toHtmlEscaped()));
        description->setAccessibleName(message.arg(name));
    }
    copy->addWidget(description);

    auto* footer = new QWidget(this);
    footer->setObjectName(QStringLiteral("VelaSaveChangesFooter"));
    auto* actions = new QHBoxLayout(footer);
    actions->setContentsMargins(24, 14, 24, 14);
    actions->setSpacing(8);
    root->addWidget(footer);

    auto* discard = new QPushButton(tr("&Discard changes"), footer);
    discard->setObjectName(QStringLiteral("VelaSaveChangesDiscard"));
    actions->addWidget(discard);
    actions->addStretch();

    auto* cancel = new QPushButton(tr("Cancel"), footer);
    cancel->setProperty("secondary", true);
    actions->addWidget(cancel);

    auto* save = new QPushButton(tr("&Save"), footer);
    save->setProperty("primary", true);
    actions->addWidget(save);

    connect(discard, &QPushButton::clicked, this, [this] { done(Discard); });
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(save, &QPushButton::clicked, this, &QDialog::accept);

    setTabOrder(discard, cancel);
    setTabOrder(cancel, save);
    save->setDefault(true);
    save->setFocus();
}

} // namespace vela
