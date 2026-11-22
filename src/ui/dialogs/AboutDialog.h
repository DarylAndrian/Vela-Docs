// ════════════════════════════════════════════════════════════════════════════
//  AboutDialog.h
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QDialog>

namespace vela {

class AboutDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AboutDialog(QWidget* parent = nullptr);
};

} // namespace vela
