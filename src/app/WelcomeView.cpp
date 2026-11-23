#include "app/WelcomeView.h"

#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace vela {

WelcomeView::WelcomeView(QWidget* parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("VelaWelcome"));

    auto* scroll = new QScrollArea(this);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(scroll);

    auto* canvas = new QWidget;
    canvas->setObjectName(QStringLiteral("VelaWelcomeCanvas"));
    canvas->setAttribute(Qt::WA_StyledBackground);
    auto* centering = new QHBoxLayout(canvas);
    centering->setContentsMargins(40, 48, 40, 40);
    auto* content = new QWidget(canvas);
    content->setMaximumWidth(920);
    centering->addWidget(content, 1, Qt::AlignTop);
    scroll->setWidget(canvas);

    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    auto* banner = new QFrame(content);
    banner->setObjectName(QStringLiteral("VelaWelcomeBanner"));
    auto* bannerLayout = new QHBoxLayout(banner);
    bannerLayout->setContentsMargins(36, 32, 36, 32);
    bannerLayout->setSpacing(28);

    auto* introduction = new QVBoxLayout;
    introduction->setSpacing(0);
    auto* heading = new QLabel(tr("Start writing"), banner);
    heading->setObjectName(QStringLiteral("VelaWelcomeHero"));
    introduction->addStretch();
    introduction->addWidget(heading);
    introduction->addSpacing(8);

    auto* subtitle = new QLabel(tr("Create a blank document or open a Word file."), banner);
    subtitle->setObjectName(QStringLiteral("VelaWelcomeTagline"));
    subtitle->setWordWrap(true);
    introduction->addWidget(subtitle);
    introduction->addSpacing(24);

    auto* actions = new QHBoxLayout;
    actions->setSpacing(10);
    auto* newButton = new QPushButton(tr("New document"), banner);
    newButton->setObjectName(QStringLiteral("VelaWelcomeNew"));
    newButton->setCursor(Qt::PointingHandCursor);
    connect(newButton, &QPushButton::clicked, this, &WelcomeView::newRequested);
    actions->addWidget(newButton);

    auto* openButton = new QPushButton(tr("Open a file…"), banner);
    openButton->setObjectName(QStringLiteral("VelaWelcomeOpen"));
    openButton->setCursor(Qt::PointingHandCursor);
    connect(openButton, &QPushButton::clicked, this, &WelcomeView::openRequested);
    actions->addWidget(openButton);
    actions->addStretch();
    introduction->addLayout(actions);
    introduction->addStretch();
    bannerLayout->addLayout(introduction, 1);

    const QIcon icon(QStringLiteral(":/icons/vela-docs.png"));
    auto* artwork = new QLabel(banner);
    artwork->setPixmap(icon.pixmap(QSize(154, 154), devicePixelRatioF()));
    bannerLayout->addWidget(artwork, 0, Qt::AlignRight | Qt::AlignVCenter);
    contentLayout->addWidget(banner);
    contentLayout->addSpacing(32);

    auto* recentHeading = new QLabel(tr("Recent documents"), content);
    recentHeading->setObjectName(QStringLiteral("VelaWelcomeSection"));
    contentLayout->addWidget(recentHeading);
    contentLayout->addSpacing(12);

    auto* recentPanel = new QFrame(content);
    recentPanel->setObjectName(QStringLiteral("VelaWelcomeRecent"));
    m_recentFilesLayout = new QVBoxLayout(recentPanel);
    m_recentFilesLayout->setContentsMargins(10, 10, 10, 10);
    m_recentFilesLayout->setSpacing(4);
    contentLayout->addWidget(recentPanel);
}

void WelcomeView::setRecentFiles(const QStringList& paths)
{
    while (auto* item = m_recentFilesLayout->takeAt(0)) {
        item->widget()->hide();
        item->widget()->deleteLater();
        delete item;
    }

    if (paths.isEmpty()) {
        auto* empty = new QLabel(tr("Documents you open will appear here."), this);
        empty->setObjectName(QStringLiteral("VelaWelcomeEmpty"));
        m_recentFilesLayout->addWidget(empty);
        return;
    }

    const QIcon icon(QStringLiteral(":/icons/vela-docs.png"));
    for (const auto& path : paths.first(qMin(5, paths.size()))) {
        const QString name = QFileInfo(path).fileName();
        auto* button = new QPushButton(icon, name, this);
        button->setObjectName(QStringLiteral("VelaRecentFileCard"));
        button->setIconSize(QSize(22, 22));
        button->setCursor(Qt::PointingHandCursor);
        button->setToolTip(path);
        button->setAccessibleName(name);
        m_recentFilesLayout->addWidget(button);
        button->ensurePolished();
        button->setText(button->fontMetrics().elidedText(name, Qt::ElideMiddle, 560));
        connect(button, &QPushButton::clicked, this, [this, path] {
            emit recentRequested(path);
        });
    }
}

} // namespace vela
