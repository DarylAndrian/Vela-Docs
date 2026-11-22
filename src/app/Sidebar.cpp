// ════════════════════════════════════════════════════════════════════════════
//  Sidebar.cpp
// ════════════════════════════════════════════════════════════════════════════
#include "app/Sidebar.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QStyle>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

namespace vela {

Sidebar::Sidebar(QWidget* parent)
    : QDockWidget(tr("Sidebar"), parent)
{
    setObjectName(QStringLiteral("VelaSidebar"));
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    setMinimumWidth(240);
    setMaximumWidth(400);

    buildContents();
}

void Sidebar::buildContents()
{
    QWidget* container = new QWidget(this);
    container->setObjectName(QStringLiteral("VelaSidebarContents"));
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_tabs = new QTabWidget(container);
    m_tabs->setObjectName(QStringLiteral("VelaSidebarTabs"));
    m_tabs->setTabPosition(QTabWidget::North);
    m_tabs->setDocumentMode(true);

    // ── Outline tab ──
    QWidget* outlinePage = new QWidget();
    QVBoxLayout* ol = new QVBoxLayout(outlinePage);
    ol->setContentsMargins(0, 8, 0, 8);
    ol->setSpacing(0);

    m_outline = new QListWidget(outlinePage);
    m_outline->setObjectName(QStringLiteral("VelaOutlineList"));
    m_outline->addItems({
        tr("1. Executive Summary"),
        tr("Key Highlights"),
        tr("2. Financial Performance"),
        tr("3. Market Expansion"),
        tr("4. Q4 Projections"),
    });
    connect(m_outline, &QListWidget::itemActivated, this,
            [this](QListWidgetItem* it) {
                emit outlineItemActivated(it->text());
            });
    ol->addWidget(m_outline);

    QPushButton* btnAdd = new QPushButton(tr("Add Section"), outlinePage);
    btnAdd->setProperty("secondary", true);
    btnAdd->setFlat(true);
    ol->addWidget(btnAdd);

    m_tabs->addTab(outlinePage, QApplication::style()->standardIcon(QStyle::SP_FileDialogInfoView),
                   tr("Outline"));

    // ── Comments tab ──
    QWidget* commentsPage = new QWidget();
    QVBoxLayout* cl = new QVBoxLayout(commentsPage);
    cl->setContentsMargins(16, 16, 16, 16);
    QLabel* clPlaceholder = new QLabel(
        tr("No comments yet.\n\nComments anchor to text selections.\n\n(Coming in Phase 5)"),
        commentsPage);
    clPlaceholder->setAlignment(Qt::AlignCenter);
    clPlaceholder->setStyleSheet(QStringLiteral("color: #777587; font-style: italic;"));
    clPlaceholder->setWordWrap(true);
    cl->addWidget(clPlaceholder);
    m_tabs->addTab(commentsPage,
                   QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation),
                   tr("Comments"));

    // ── Explore tab ──
    QWidget* explorePage = new QWidget();
    QVBoxLayout* el = new QVBoxLayout(explorePage);
    el->setContentsMargins(16, 16, 16, 16);
    QLabel* elPlaceholder = new QLabel(
        tr("Select text and explore definitions.\n\n(Coming in Phase 5)"),
        explorePage);
    elPlaceholder->setAlignment(Qt::AlignCenter);
    elPlaceholder->setStyleSheet(QStringLiteral("color: #777587; font-style: italic;"));
    elPlaceholder->setWordWrap(true);
    el->addWidget(elPlaceholder);
    m_tabs->addTab(explorePage,
                   QApplication::style()->standardIcon(QStyle::SP_MessageBoxQuestion),
                   tr("Explore"));

    // ── History tab ──
    QWidget* historyPage = new QWidget();
    QVBoxLayout* hl = new QVBoxLayout(historyPage);
    hl->setContentsMargins(16, 16, 16, 16);
    QLabel* hlPlaceholder = new QLabel(
        tr("Local revision history.\n\nSnapshots every 5 minutes.\n\n(Coming in Phase 5)"),
        historyPage);
    hlPlaceholder->setAlignment(Qt::AlignCenter);
    hlPlaceholder->setStyleSheet(QStringLiteral("color: #777587; font-style: italic;"));
    hlPlaceholder->setWordWrap(true);
    hl->addWidget(hlPlaceholder);
    m_tabs->addTab(historyPage,
                   QApplication::style()->standardIcon(QStyle::SP_BrowserReload),
                   tr("History"));

    layout->addWidget(m_tabs);

    setWidget(container);
}

void Sidebar::showView(int index)
{
    if (m_tabs && index >= 0 && index < m_tabs->count()) {
        m_tabs->setCurrentIndex(index);
    }
}

void Sidebar::showOutline()
{
    showView(0);
}

} // namespace vela