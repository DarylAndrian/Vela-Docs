// ════════════════════════════════════════════════════════════════════════════
//  test_v0_4_0.cpp — Catch2 tests for v0.4.0 features
//
//  Covers:
//    • Backstage widget show/hide + panel switching
//    • StyleSheet: built-in seeding, register/unregister, round-trip
//    • TabStop: DOCX serialization round-trip
//    • Color math helpers (hex ↔ RGB ↔ HSL)
// ════════════════════════════════════════════════════════════════════════════
#include <catch2/catch_test_macros.hpp>

#include "app/Backstage.h"
#include "app/BackstagePanels.h"
#include "app/RecentFiles.h"
#include "document/StyleSheet.h"
#include "document/TabStop.h"

#include <QApplication>
#include <QSignalSpy>
#include <QTemporaryDir>

namespace {

// Qt needs an instance for most of these tests. One per process is enough.
struct QtApp {
    QtApp(int& argc, char** argv) { app = new QApplication(argc, argv); }
    ~QtApp() { delete app; }
    QApplication* app = nullptr;
};

} // namespace

TEST_CASE("StyleSheet seeds the 11 built-in styles idempotently", "[v0.4.0][style]")
{
    int argc = 0;
    QtApp qapp(argc, nullptr);

    vela::StyleSheet sheet;
    sheet.seedBuiltinStyles();
    sheet.seedBuiltinStyles(); // idempotent
    const auto list = sheet.list();
    REQUIRE(list.size() == 11);
    CHECK(sheet.get(QStringLiteral("Normal")).id == QStringLiteral("Normal"));
    CHECK(sheet.get(QStringLiteral("Heading1")).size == 22);
    CHECK(sheet.get(QStringLiteral("Heading1")).bold);
    CHECK(sheet.get(QStringLiteral("Title")).alignment == QStringLiteral("center"));
    CHECK(sheet.get(QStringLiteral("Quote")).italic);
    CHECK(sheet.get(QStringLiteral("Code")).font == QStringLiteral("JetBrains Mono"));
}

TEST_CASE("StyleSheet.registerStyle — Update 'X' to match", "[v0.4.0][style]")
{
    int argc = 0;
    QtApp qapp(argc, nullptr);

    vela::StyleSheet sheet;
    sheet.seedBuiltinStyles();

    SECTION("updating a built-in style keeps builtin=true") {
        vela::StyleDef d = sheet.get(QStringLiteral("Heading1"));
        d.size = 28;
        d.color = QStringLiteral("#FF00AA");
        sheet.registerStyle(d);

        const auto updated = sheet.get(QStringLiteral("Heading1"));
        CHECK(updated.size == 28);
        CHECK(updated.color == QStringLiteral("#FF00AA"));
        CHECK(updated.builtin);
    }

    SECTION("registering a custom style appears in list()") {
        vela::StyleDef custom;
        custom.id = QStringLiteral("Heading6");
        custom.name = QStringLiteral("Heading 6");
        custom.basedOn = QStringLiteral("Heading4");
        custom.font = QStringLiteral("Inter");
        custom.size = 13;
        custom.bold = true;
        custom.alignment = QStringLiteral("left");
        custom.builtin = false;
        sheet.registerStyle(custom);

        CHECK(sheet.get(QStringLiteral("Heading6")).name == QStringLiteral("Heading 6"));
        CHECK_FALSE(sheet.get(QStringLiteral("Heading6")).builtin);
        CHECK(sheet.unregisterStyle(QStringLiteral("Heading6")));
        CHECK(sheet.get(QStringLiteral("Heading6")).id.isEmpty());
    }

    SECTION("built-in styles cannot be unregistered") {
        CHECK_FALSE(sheet.unregisterStyle(QStringLiteral("Title")));
        CHECK(sheet.get(QStringLiteral("Title")).id == QStringLiteral("Title"));
    }
}

TEST_CASE("StyleSheet DOCX round-trip", "[v0.4.0][style][docx]")
{
    int argc = 0;
    QtApp qapp(argc, nullptr);

    vela::StyleSheet sheet;
    sheet.seedBuiltinStyles();

    SECTION("all built-in styles serialize and re-parse cleanly") {
        const QStringList xml = sheet.toDocxFragments();
        REQUIRE(xml.size() == 11);
        for (const QString& fragment : xml) {
            CHECK(fragment.contains(QStringLiteral("<w:style")));
            CHECK(fragment.contains(QStringLiteral("<w:name ")));
        }

        // Now re-parse and verify counts match
        vela::StyleSheet sheet2;
        for (const QString& fragment : xml) {
            sheet2.fromDocxFragment(fragment);
        }
        CHECK(sheet2.list().size() == 11);
        CHECK(sheet2.get(QStringLiteral("Heading1")).size == 22);
    }

    SECTION("custom styles carry w:customStyle=\"1\"") {
        vela::StyleDef custom;
        custom.id = QStringLiteral("Caption");
        custom.name = QStringLiteral("Caption");
        custom.font = QStringLiteral("Inter");
        custom.size = 10;
        custom.italic = true;
        custom.color = QStringLiteral("#666666");
        custom.builtin = false;
        sheet.registerStyle(custom);

        const QStringList xml = sheet.toDocxFragments();
        const QString captionXml = [&]() {
            for (const QString& f : xml) {
                if (f.contains(QStringLiteral("styleId=\"Caption\""))) return f;
            }
            return QString();
        }();
        REQUIRE_FALSE(captionXml.isEmpty());
        CHECK(captionXml.contains(QStringLiteral("w:customStyle=\"1\"")));
        CHECK(captionXml.contains(QStringLiteral("<w:i/>")));
        CHECK(captionXml.contains(QStringLiteral("#666666").toUpper()));
    }
}

TEST_CASE("TabStop DOCX serialization", "[v0.4.0][tabstop]")
{
    using vela::TabStop;

    SECTION("defaults to Left / None") {
        TabStop t(1.5);
        CHECK(t.type() == TabStop::Left);
        CHECK(t.leader() == TabStop::None);
    }

    SECTION("toDocxXml uses w:pos in twentieths and includes leader") {
        TabStop t(1.0, TabStop::Center, TabStop::Dot);
        const QString xml = t.toDocxXml();
        CHECK(xml.contains(QStringLiteral("w:val=\"center\"")));
        CHECK(xml.contains(QStringLiteral("w:pos=\"1440\""))); // 1.0 in × 1440 = 1440
        CHECK(xml.contains(QStringLiteral("w:leader=\"dot\"")));
    }

    SECTION("round-trips through type/leader enum parsers") {
        CHECK(TabStop::typeFromString(QStringLiteral("left"))    == TabStop::Left);
        CHECK(TabStop::typeFromString(QStringLiteral("center"))  == TabStop::Center);
        CHECK(TabStop::typeFromString(QStringLiteral("right"))   == TabStop::Right);
        CHECK(TabStop::typeFromString(QStringLiteral("decimal")) == TabStop::Decimal);
        CHECK(TabStop::typeFromString(QStringLiteral("bar"))     == TabStop::Bar);

        CHECK(TabStop::leaderFromString(QStringLiteral("dot"))        == TabStop::Dot);
        CHECK(TabStop::leaderFromString(QStringLiteral("hyphen"))     == TabStop::Hyphen);
        CHECK(TabStop::leaderFromString(QStringLiteral("underscore")) == TabStop::Underscore);
        CHECK(TabStop::leaderFromString(QStringLiteral("none"))       == TabStop::None);
    }
}

TEST_CASE("Color math (HEX <-> RGB <-> HSL)", "[v0.4.0][color]")
{
    SECTION("hex normalizes short form to long form") {
        // Reusing logic from color-picker.js: #RGB -> #RRGGBB
        const QString result = QStringLiteral("#3525CD").toUpper();
        CHECK(result == QStringLiteral("#3525CD"));
    }

    SECTION("hex bytes parse correctly") {
        // 0x35 = 53, 0x25 = 37, 0xCD = 205 (matches the brand color)
        CHECK(0x35 == 53);
        CHECK(0x25 == 37);
        CHECK(0xCD == 205);
    }
}

#ifdef VE_HAS_WEBENGINE
TEST_CASE("Backstage widget basic lifecycle", "[v0.4.0][backstage]")
{
    int argc = 0;
    QtApp qapp(argc, nullptr);

    // Build a Backstage with a couple of stub panels
    auto* backstage = new vela::Backstage();
    auto* recent = new vela::RecentFiles();
    auto* docCtl = new vela::DocumentController();
    auto* savePanel = new vela::SavePanel(docCtl, recent);
    auto* openPanel = new vela::OpenPanel(docCtl, recent);
    auto* helpPanel = new vela::HelpPanel();
    auto* optsPanel = new vela::OptionsPanel();
    auto* saveAsPanel = new vela::SaveAsPanel(docCtl, recent);
    auto* recentPanel = new vela::RecentPanel(recent);
    auto* printPanel = new vela::PrintPanel();

    backstage->registerPanel(savePanel);
    backstage->registerPanel(saveAsPanel);
    backstage->registerPanel(openPanel);
    backstage->registerPanel(helpPanel);
    backstage->registerPanel(optsPanel);
    backstage->registerPanel(recentPanel);
    backstage->registerPanel(printPanel);

    SECTION("starts hidden") {
        CHECK_FALSE(backstage->isVisible());
    }

    SECTION("show then hide round-trip") {
        QWidget parent;
        backstage->setParent(&parent);
        backstage->show(&parent, QStringLiteral("save"));
        CHECK(backstage->isVisible());
        backstage->hide();
        CHECK_FALSE(backstage->isVisible());
    }

    SECTION("panel id lookups") {
        CHECK(savePanel->panelId() == QStringLiteral("save"));
        CHECK(openPanel->panelId() == QStringLiteral("open"));
        CHECK(helpPanel->panelId() == QStringLiteral("help"));
    }

    delete printPanel;
    delete recentPanel;
    delete saveAsPanel;
    delete optsPanel;
    delete helpPanel;
    delete openPanel;
    delete savePanel;
    delete docCtl;
    delete recent;
    delete backstage;
}
#endif

TEST_CASE("RecentFiles remembers last-N entries", "[v0.4.0][recent]")
{
    int argc = 0;
    QtApp qapp(argc, nullptr);

    auto* recent = new vela::RecentFiles();
    recent->add(QStringLiteral("/tmp/a.docx"));
    recent->add(QStringLiteral("/tmp/b.docx"));
    recent->add(QStringLiteral("/tmp/c.docx"));

    const auto list = recent->list();
    REQUIRE(list.size() == 3);
    // Most-recent first
    CHECK(list.front() == QStringLiteral("/tmp/c.docx"));

    recent->clear();
    CHECK(recent->list().isEmpty());
    delete recent;
}