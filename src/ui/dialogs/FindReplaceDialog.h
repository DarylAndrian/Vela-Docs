// ════════════════════════════════════════════════════════════════════════════
//  FindReplaceDialog.h — Phase 2
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QDialog>

namespace vela {

class FindReplaceDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FindReplaceDialog(QWidget* parent = nullptr);
};

} // namespace vela
