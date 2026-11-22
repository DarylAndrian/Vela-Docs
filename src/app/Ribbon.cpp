// ════════════════════════════════════════════════════════════════════════════
//  Ribbon.cpp — Compact formatting toolbar — v0.4.0
//
//  v0.4.0 changes:
//    • Switched from text labels (L/C/R/J/UL/OL/H1-H5) to bundled SVG icons
//      in resources/icons/material/*.svg (Material Symbols Outlined set)
//    • Each makeButton() call now gets an icon via QIcon and uses
//      Qt::ToolButtonIconOnly mode
//    • Kept text label fallback when the SVG asset is missing
// ════════════════════════════════════════════════════════════════════════════
#include "app/Ribbon.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QComboBox>
#include <QFontDatabase>
#include <QIcon>
#include <QLabel>
#include <QStyle>
#include <QToolButton>

namespace vela {

namespace {
    /// Look up a Material Symbols SVG icon by its short name.
    /// Falls back to the supplied text label if the asset is missing.
    QIcon iconOrLabel(const QString& svgName, const QString& fallbackText)
    {
        QIcon icon(QStringLiteral(":/icons/material/") + svgName + QStringLiteral(".svg"));
        if (!icon.isNull()) return icon;
        return QIcon(); // empty — caller will show the text label
    }

    QAction* makeButton(QToolBar* bar, const QString& svgName,
                        const QString& text, const QString& tooltip,
                        const QString& cmd, bool checkable = false)
    {
        QIcon icon = iconOrLabel(svgName, text);
        QAction* a = new QAction(icon, text, bar);
        a->setToolTip(tooltip);
        if (checkable) a->setCheckable(true);
        bar->addAction(a);
        QObject::connect(a, &QAction::triggered, bar, [bar, cmd]() {
            Q_UNUSED(bar);
            qobject_cast<Ribbon*>(bar)->emitCommand(cmd);
        });
        return a;
    }
}

Ribbon::Ribbon(QWidget* parent)
    : QToolBar(tr("Ribbon"), parent)
{
    setObjectName(QStringLiteral("VelaRibbon"));
    setMovable(false);
    setFloatable(false);
    setIconSize(QSize(18, 18));
    // v0.4.0: icon-only buttons (SVG icons)
    setToolButtonStyle(Qt::ToolButtonIconOnly);

    // ── Font family ─────────────────────────────────────────────────
    m_fontFamily = new QComboBox(this);
    m_fontFamily->setEditable(true);
    m_fontFamily->setMinimumWidth(160);
    m_fontFamily->setToolTip(tr("Font family"));

    QStringList families = QFontDatabase::families();
    const QStringList preferred{
        QStringLiteral("Inter"), QStringLiteral("Source Serif 4"),
        QStringLiteral("Source Serif Pro"), QStringLiteral("Georgia"),
        QStringLiteral("Times New Roman"), QStringLiteral("Calibri"),
        QStringLiteral("Arial"), QStringLiteral("Helvetica"),
        QStringLiteral("JetBrains Mono"), QStringLiteral("Cascadia Code"),
        QStringLiteral("Consolas"), QStringLiteral("Courier New"),
    };
    QStringList ordered;
    for (const QString& p : preferred) {
        if (families.contains(p)) ordered << p;
    }
    for (const QString& f : families) {
        if (!ordered.contains(f)) ordered << f;
    }
    m_fontFamily->addItems(ordered);
    m_fontFamily->setCurrentText(QStringLiteral("Source Serif 4"));
    connect(m_fontFamily, &QComboBox::currentTextChanged,
            this, [this](const QString& f) {
                emit formatCommand(QStringLiteral("fontFamily:%1").arg(f));
            });
    addWidget(m_fontFamily);

    // ── Font size ──────────────────────────────────────────────────
    m_fontSize = new QComboBox(this);
    m_fontSize->setEditable(true);
    m_fontSize->setMinimumWidth(60);
    m_fontSize->setToolTip(tr("Font size"));
    const QStringList sizes{
        QStringLiteral("9"), QStringLiteral("10"), QStringLiteral("11"),
        QStringLiteral("12"), QStringLiteral("14"), QStringLiteral("16"),
        QStringLiteral("18"), QStringLiteral("20"), QStringLiteral("24"),
        QStringLiteral("28"), QStringLiteral("32"), QStringLiteral("36"),
        QStringLiteral("48"), QStringLiteral("72"),
    };
    m_fontSize->addItems(sizes);
    m_fontSize->setCurrentText(QStringLiteral("12"));
    connect(m_fontSize, &QComboBox::currentTextChanged,
            this, [this](const QString& s) {
                emit formatCommand(QStringLiteral("fontSize:%1").arg(s));
            });
    addWidget(m_fontSize);

    addSeparator();

    // ── B/I/U/S ───────────────────────────────────────────────────
    makeButton(this, QStringLiteral("format_bold"),          QStringLiteral("B"),
               tr("Bold (Ctrl+B)"),          QStringLiteral("bold"),          true);
    makeButton(this, QStringLiteral("format_italic"),        QStringLiteral("I"),
               tr("Italic (Ctrl+I)"),        QStringLiteral("italic"),        true);
    makeButton(this, QStringLiteral("format_underlined"),    QStringLiteral("U"),
               tr("Underline (Ctrl+U)"),    QStringLiteral("underline"),     true);
    makeButton(this, QStringLiteral("format_strikethrough"), QStringLiteral("S"),
               tr("Strikethrough"),          QStringLiteral("strike"),        true);

    addSeparator();

    // ── Colors ────────────────────────────────────────────────────
    makeButton(this, QStringLiteral("format_color_text"), QStringLiteral("A"),
               tr("Text color"),               QStringLiteral("textColor"));
    makeButton(this, QStringLiteral("border_color"),     QStringLiteral("H"),
               tr("Highlight color"),          QStringLiteral("highlight"));

    addSeparator();

    // ── Paragraph style (dropdown) ───────────────────────────────
    {
        QComboBox* style = new QComboBox(this);
        style->setMinimumWidth(140);
        style->setToolTip(tr("Paragraph style"));
        style->addItems({
            tr("Normal Text"),
            tr("Heading 1"), tr("Heading 2"), tr("Heading 3"),
            tr("Heading 4"), tr("Heading 5"), tr("Heading 6"),
            tr("Title"), tr("Subtitle"),
            tr("Quote"), tr("Code"),
        });
        style->setCurrentIndex(0);
        connect(style, qOverload<int>(&QComboBox::currentIndexChanged),
                this, [this, style](int idx) {
                    const QStringList styles{
                        QStringLiteral("p"),      // Normal
                        QStringLiteral("h1"), QStringLiteral("h2"),
                        QStringLiteral("h3"), QStringLiteral("h4"),
                        QStringLiteral("h5"), QStringLiteral("h6"),
                        QStringLiteral("title"),
                        QStringLiteral("subtitle"),
                        QStringLiteral("blockquote"),
                        QStringLiteral("pre"),
                    };
                    if (idx >= 0 && idx < styles.size()) {
                        QString cmd = styles[idx];
                        if (cmd == QStringLiteral("p"))
                            emit formatCommand(QStringLiteral("heading:p"));
                        else if (cmd == QStringLiteral("title") || cmd == QStringLiteral("subtitle"))
                            emit formatCommand(cmd);
                        else
                            emit formatCommand(QStringLiteral("heading:") + cmd);
                    }
                });
        addWidget(style);
    }

    addSeparator();

    // ── Alignment (v0.4.0: SVG icons, not L/C/R/J text) ───────────
    {
        QAction* left = makeButton(this, QStringLiteral("format_align_left"),
                                   QStringLiteral("Left"), tr("Align left"),
                                   QStringLiteral("align:left"),   true);
        QAction* center = makeButton(this, QStringLiteral("format_align_center"),
                                     QStringLiteral("Center"), tr("Align center"),
                                     QStringLiteral("align:center"), true);
        QAction* right = makeButton(this, QStringLiteral("format_align_right"),
                                    QStringLiteral("Right"), tr("Align right"),
                                    QStringLiteral("align:right"),  true);
        QAction* justify = makeButton(this, QStringLiteral("format_align_justify"),
                                     QStringLiteral("Justify"), tr("Justify"),
                                     QStringLiteral("align:justify"), true);
        QActionGroup* grp = new QActionGroup(this);
        grp->addAction(left);
        grp->addAction(center);
        grp->addAction(right);
        grp->addAction(justify);
        grp->setExclusive(true);
    }

    addSeparator();

    // ── Lists (v0.4.0: SVG icons) ──────────────────────────────────
    makeButton(this, QStringLiteral("format_list_bulleted"), QStringLiteral("UL"),
               tr("Bullet list (Ctrl+Shift+L)"),   QStringLiteral("bulletList"));
    makeButton(this, QStringLiteral("format_list_numbered"), QStringLiteral("OL"),
               tr("Numbered list (Ctrl+Shift+O)"), QStringLiteral("orderedList"));
    makeButton(this, QStringLiteral("check_box_outline_blank"), QStringLiteral("☐"),
               tr("Task list (Ctrl+Shift+7)"),     QStringLiteral("taskList"));

    addSeparator();

    // ── Indent ────────────────────────────────────────────────────
    makeButton(this, QStringLiteral("format_indent_increase"), QStringLiteral("→|"),
               tr("Indent (Ctrl+])"), QStringLiteral("indent"));
    makeButton(this, QStringLiteral("format_indent_decrease"), QStringLiteral("|←"),
               tr("Outdent (Ctrl+[)"), QStringLiteral("outdent"));

    addSeparator();

    // ── Clear formatting ──────────────────────────────────────────
    makeButton(this, QStringLiteral("format_clear"), QStringLiteral("Tx"),
               tr("Clear formatting"), QStringLiteral("clearFormatting"));
}

void Ribbon::emitCommand(const QString& cmd)
{
    emit formatCommand(cmd);
}

} // namespace vela