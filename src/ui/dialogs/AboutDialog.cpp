#include "ui/dialogs/AboutDialog.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace vela {

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("About Vela Docs"));
    setMinimumWidth(420);

    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* title = new QLabel(QStringLiteral("<h2>Vela Docs " VE_VERSION "</h2>"), this);
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    layout->addWidget(new QLabel(tr(
        "<p>A modern word processor combining MS Word + Google Docs features.</p>"
        "<p>Built with Qt %1 and C++20.</p>"
        "<p>&copy; 2026 Vela Docs Contributors. Licensed under MIT.</p>")
        .arg(QStringLiteral(QT_VERSION_STR)), this));

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Close"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    layout->addWidget(buttons);
}

} // namespace vela
