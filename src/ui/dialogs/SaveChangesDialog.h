#pragma once

#include <QDialog>

namespace vela {

class SaveChangesDialog : public QDialog
{
    Q_OBJECT

public:
    enum Result { Cancel = QDialog::Rejected, Save = QDialog::Accepted, Discard };

    SaveChangesDialog(QWidget* parent, const QString& heading, const QString& documentName);
};

} // namespace vela
