// ════════════════════════════════════════════════════════════════════════════
//  ImagePropertiesDialog.h — Image size / wrap / alt text (v0.3.2 — MAJOR-06)
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QDialog>

namespace vela {

struct ImageOptions
{
    int     width       = 0;        ///< 0 means "auto / intrinsic"
    int     height      = 0;
    bool    keepAspect  = true;
    QString wrap        = QStringLiteral("inline");   // inline/square/tight/behind/front
    QString altText;
    QString caption;
    int     rotationDeg = 0;
};

class ImagePropertiesDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ImagePropertiesDialog(const ImageOptions& initial, QWidget* parent = nullptr);
    ImageOptions value() const { return m_value; }

private:
    ImageOptions m_value;
};

} // namespace vela
