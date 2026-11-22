// ════════════════════════════════════════════════════════════════════════════
//  test_v0_5_0.cpp — Comprehensive tests for v0.5.0
//
//  Covers:
//    • PageStyleSheet (built-in seeding + custom registration)
//    • SectionStack with multiple sections (landscape + portrait + odd-even)
//    • PageStyleSheet::toXmlStyleEntry / fromDocxFragment round-trip
//    • LineNumberSettings every flag combo
//    • ColumnSettings equalWidth / non-equalWidth / separator flag
//    • DropCapSettings drop vs margin + lines + font override
//    • DocumentProtection 5 edit modes + label + DOCX value mapping
//    • Full DOCX round-trip via DocxExporter + DocxImporter — paragraph
//      + page style + section + column + line numbering + drop cap
//      + document protection all in one body
// ════════════════════════════════════════════════════════════════════════════
#include <catch2/catch_test_macros.hpp>

#include "document/PageStyle.h"
#include "formats/DocxImporter.h"
#include "formats/DocxExporter.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QUuid>

using namespace vela;

namespace {

QString tmpPath(const QString& suffix)
{
    return QDir::tempPath() + QStringLiteral("/vela-v050-")
        + QString::number(QDateTime::currentMSecsSinceEpoch())
        + QLatin1Char('-')
        + QUuid::createUuid().toString(QUuid::WithoutBraces).left(6)
        + suffix;
}

QJsonObject makeParagraph(const QString& text)
{
    QJsonObject p;
    p.insert(QStringLiteral("type"), QStringLiteral("paragraph"));
    QJsonArray content;
    QJsonObject r;
    r.insert(QStringLiteral("type"), QStringLiteral("text"));
    r.insert(QStringLiteral("text"), text);
    content.append(r);
    p.insert(QStringLiteral("content"), content);
    return p;
}

} // anon

// ══════════════════════════════════════════════════════════════════════════
//  PageStyleSheet
// ══════════════════════════════════════════════════════════════════════════

TEST_CASE("PageStyleSheet seeds three built-ins on construction", "[v0.5.0][pagestyle]")
{
    PageStyleSheet ss;
    const QList<PageStyle> all = ss.list();
    CHECK(all.size() >= 3);
    QSet<QString> ids;
    for (const auto& p : all) ids.insert(p.id);
    CHECK(ids.contains(QStringLiteral("PageStyleDefault")));
    CHECK(ids.contains(QStringLiteral("PageStyleLandscape")));
    CHECK(ids.contains(QStringLiteral("PageStyleTitlePage")));

    const PageStyle def = ss.get(QStringLiteral("PageStyleDefault"));
    CHECK(def.widthTwips  == 12240);
    CHECK(def.heightTwips == 15840);
    CHECK(def.orientation == PageOrientation::Portrait);

    const PageStyle ls = ss.get(QStringLiteral("PageStyleLandscape"));
    CHECK(ls.orientation == PageOrientation::Landscape);
    CHECK(ls.widthTwips  == 15840);   // 11"
    CHECK(ls.heightTwips == 12240);   // 8.5"

    const PageStyle tp = ss.get(QStringLiteral("PageStyleTitlePage"));
    CHECK(tp.titlePg == true);
    CHECK(tp.differentFirstPage == true);
}

TEST_CASE("PageStyleSheet refuses to unregister built-ins", "[v0.5.0][pagestyle]")
{
    PageStyleSheet ss;
    CHECK_FALSE(ss.unregisterStyle(QStringLiteral("PageStyleDefault")));
    CHECK_FALSE(ss.unregisterStyle(QStringLiteral("PageStyleLandscape")));
    CHECK_FALSE(ss.unregisterStyle(QStringLiteral("PageStyleTitlePage")));
}

TEST_CASE("PageStyleSheet roundtrips to/from DOCX fragments", "[v0.5.0][pagestyle][roundtrip]")
{
    PageStyleSheet ss;
    PageStyle custom;
    custom.id          = QStringLiteral("PageStyleCustom1");
    custom.name        = QStringLiteral("Custom");
    custom.orientation = PageOrientation::Landscape;
    custom.widthTwips  = 16838;   // 11.69" A4-ish
    custom.heightTwips = 11906;
    custom.marginTop    = 1134;
    custom.marginBottom = 1134;
    custom.mirrorMargins = true;
    custom.differentFirstPage = true;
    custom.titlePg    = true;
    ss.registerStyle(custom);

    const QStringList fragments = ss.toDocxFragments();
    CHECK(fragments.size() >= 4);

    bool found = false;
    for (const auto& f : fragments) {
        if (f.contains(QStringLiteral("PageStyleCustom1"))) found = true;
    }
    CHECK(found);

    // Round-trip into a fresh sheet
    PageStyleSheet ss2;
    for (const auto& f : fragments) ss2.fromDocxFragment(f);
    const PageStyle got = ss2.get(QStringLiteral("PageStyleCustom1"));
    CHECK(got.name        == QStringLiteral("Custom"));
    CHECK(got.orientation == PageOrientation::Landscape);
    CHECK(got.widthTwips  == 16838);
    CHECK(got.heightTwips == 11906);
    CHECK(got.marginTop   == 1134);
    CHECK(got.mirrorMargins  == true);
    CHECK(got.differentFirstPage == true);
}

// ══════════════════════════════════════════════════════════════════════════
//  LineNumberSettings
// ══════════════════════════════════════════════════════════════════════════

TEST_CASE("LineNumberSettings emits only when enabled and round-trips", "[v0.5.0][linenum]")
{
    LineNumberSettings ln;
    CHECK(ln.toXml().isEmpty());

    ln.enabled  = true;
    ln.countBy  = 5;
    ln.startAt  = 2;
    ln.distance = 240;
    ln.restartPerPage    = true;
    ln.continuous        = false;
    ln.restartPerSection = false;
    const QString xml = ln.toXml();
    REQUIRE(xml.contains(QStringLiteral("<w:lnNumType")));
    REQUIRE(xml.contains(QStringLiteral("w:countBy=\"5\"")));
    REQUIRE(xml.contains(QStringLiteral("w:start=\"2\"")));
    REQUIRE(xml.contains(QStringLiteral("w:distance=\"240\"")));
    REQUIRE(xml.contains(QStringLiteral("w:restart=\"newPage\"")));

    LineNumberSettings parsed;
    REQUIRE(LineNumberSettings::fromXml(xml, parsed));
    CHECK(parsed.countBy     == 5);
    CHECK(parsed.startAt     == 2);
    CHECK(parsed.distance    == 240);
    CHECK(parsed.restartPerPage == true);
    CHECK(parsed.continuous     == false);
}

TEST_CASE("LineNumberSettings restart policies round-trip", "[v0.5.0][linenum]")
{
    for (const auto& mode : { QStringLiteral("newSection"),
                              QStringLiteral("newPage"),
                              QStringLiteral("continuous") }) {
        LineNumberSettings ln;
        ln.enabled = true;
        ln.countBy = 1;
        if (mode == QLatin1String("newSection"))      ln.restartPerSection = true;
        else if (mode == QLatin1String("continuous")) ln.continuous = true;
        const QString xml = ln.toXml();
        CAPTURE(mode);
        REQUIRE(xml.contains(QStringLiteral("w:restart=\"") + mode + QStringLiteral("\"")));
        LineNumberSettings back;
        REQUIRE(LineNumberSettings::fromXml(xml, back));
        if (mode == QLatin1String("newSection"))      CHECK(back.restartPerSection == true);
        else if (mode == QLatin1String("continuous")) CHECK(back.continuous        == true);
    }
}

// ══════════════════════════════════════════════════════════════════════════
//  ColumnSettings
// ══════════════════════════════════════════════════════════════════════════

TEST_CASE("ColumnSettings emits no XML when count is single", "[v0.5.0][columns]")
{
    ColumnSettings c;
    c.count = 1;
    CHECK(c.toXml().isEmpty());
}

TEST_CASE("ColumnSettings equalWidth short-form XML roundtrips", "[v0.5.0][columns]")
{
    ColumnSettings c;
    c.count = 3;
    c.space = 720;
    c.equalWidth = true;
    c.separator  = true;
    const QString xml = c.toXml();
    CAPTURE(xml);
    REQUIRE(xml.contains(QStringLiteral("<w:cols w:num=\"3\"")));
    REQUIRE(xml.contains(QStringLiteral("w:space=\"720\"")));
    REQUIRE(xml.contains(QStringLiteral("w:sep=\"1\"")));

    ColumnSettings back;
    REQUIRE(ColumnSettings::fromXml(xml, back));
    CHECK(back.count      == 3);
    CHECK(back.space      == 720);
    CHECK(back.equalWidth == true);
    CHECK(back.separator  == true);
}

TEST_CASE("ColumnSettings non-equalWidth emits <w:col> children", "[v0.5.0][columns]")
{
    ColumnSettings c;
    c.count = 2;
    c.space = 360;
    c.equalWidth = false;
    c.customWidths << 4320 << 2160;
    const QString xml = c.toXml();
    CAPTURE(xml);
    REQUIRE(xml.contains(QStringLiteral("<w:cols w:num=\"2\"")));
    REQUIRE(xml.contains(QStringLiteral("w:equalWidth=\"0\"")));
    REQUIRE(xml.contains(QStringLiteral("<w:col w:w=\"4320\"")));
    REQUIRE(xml.contains(QStringLiteral("<w:col w:w=\"2160\"")));
}

// ══════════════════════════════════════════════════════════════════════════
//  DropCapSettings
// ══════════════════════════════════════════════════════════════════════════

TEST_CASE("DropCapSettings emits nothing for None", "[v0.5.0][dropcap]")
{
    DropCapSettings d;
    d.type = DropCapSettings::Type::None;
    CHECK(d.toXml().isEmpty());
}

TEST_CASE("DropCapSettings drop / margin both round-trip", "[v0.5.0][dropcap]")
{
    for (const auto& k : { DropCapSettings::Type::Drop,
                           DropCapSettings::Type::Margin }) {
        DropCapSettings d;
        d.type  = k;
        d.lines = 4;
        d.font  = QStringLiteral("Playfair Display");
        const QString xml = d.toXml();
        CAPTURE(xml);
        const QString expectedKind = (k == DropCapSettings::Type::Drop)
                                   ? QStringLiteral("drop")
                                   : QStringLiteral("margin");
        REQUIRE(xml.contains(QStringLiteral("w:val=\"") + expectedKind + QStringLiteral("\"")));
        REQUIRE(xml.contains(QStringLiteral("w:lines=\"4\"")));
        REQUIRE(xml.contains(QStringLiteral("w:font=\"Playfair Display\"")));

        DropCapSettings back;
        REQUIRE(DropCapSettings::fromXml(xml, back));
        CHECK(back.type == k);
        CHECK(back.lines == 4);
    }
}

// ══════════════════════════════════════════════════════════════════════════
//  DocumentProtection
// ══════════════════════════════════════════════════════════════════════════

TEST_CASE("DocumentProtection mapping covers all five edit modes", "[v0.5.0][protection]")
{
    using E = DocumentProtection::Edit;
    for (auto mode : { E::NoProtection, E::ReadOnly, E::Comments,
                       E::TrackedChanges, E::Forms }) {
        CAPTURE(int(mode));
        const QString label = DocumentProtection::labelFor(mode);
        CHECK_FALSE(label.isEmpty());
        const QString docx = DocumentProtection::docxValueFor(mode);
        if (mode == E::NoProtection) CHECK(docx.isEmpty());
        else                        CHECK_FALSE(docx.isEmpty());
        CHECK(DocumentProtection::fromDocxValue(docx) == mode);
    }
}

TEST_CASE("DocumentProtection emits <w:documentProtection> only when enforced", "[v0.5.0][protection]")
{
    DocumentProtection p;
    p.mode = DocumentProtection::Edit::ReadOnly;
    CHECK(p.toXml().isEmpty());         // !enforce ⇒ nothing written

    p.enforce = true;
    const QString xml = p.toXml();
    CAPTURE(xml);
    REQUIRE(xml.contains(QStringLiteral("<w:documentProtection")));
    REQUIRE(xml.contains(QStringLiteral("w:edit=\"readOnly\"")));
    REQUIRE(xml.contains(QStringLiteral("w:enforcement=\"1\"")));

    DocumentProtection back;
    REQUIRE(DocumentProtection::fromXml(xml, back));
    CHECK(back.mode      == DocumentProtection::Edit::ReadOnly);
    CHECK(back.enforce   == true);
}

TEST_CASE("DocumentProtection carries password hash & salt", "[v0.5.0][protection]")
{
    DocumentProtection p;
    p.mode = DocumentProtection::Edit::Comments;
    p.enforce = true;
    p.passwordHash = QStringLiteral("DEADBEEF");
    p.salt = QStringLiteral("CAFEBABE");
    p.spinCount = 250000;

    const QString xml = p.toXml();
    REQUIRE(xml.contains(QStringLiteral("w:hash=\"DEADBEEF\"")));
    REQUIRE(xml.contains(QStringLiteral("w:salt=\"CAFEBABE\"")));
    REQUIRE(xml.contains(QStringLiteral("w:cryptSpinCount=\"250000\"")));

    DocumentProtection back;
    REQUIRE(DocumentProtection::fromXml(xml, back));
    CHECK(back.passwordHash == QStringLiteral("DEADBEEF"));
    CHECK(back.salt         == QStringLiteral("CAFEBABE"));
}

// ══════════════════════════════════════════════════════════════════════════
//  SectionStack
// ══════════════════════════════════════════════════════════════════════════

TEST_CASE("SectionStack: landscape + portrait sections serialize distinctly", "[v0.5.0][sections]")
{
    SectionStack s;

    SectionData portrait;
    portrait.breakKind   = PageSectionBreak::NextPage;
    portrait.orientation = PageOrientation::Portrait;
    portrait.marginTop = 1440;
    portrait.marginBottom = 1440;
    portrait.marginLeft = 1440;
    portrait.marginRight = 1440;
    s.append(portrait);

    SectionData landscape;
    landscape.breakKind   = PageSectionBreak::NextPage;
    landscape.orientation = PageOrientation::Landscape;
    landscape.marginTop = 1080;
    landscape.marginBottom = 1080;
    landscape.marginLeft = 720;
    landscape.marginRight = 720;
    landscape.titlePg = true;
    s.append(landscape);

    const QString xml = s.serializeForDocx();
    CAPTURE(xml);
    REQUIRE(xml.contains(QStringLiteral("<w:sectPr>")));
    // The portrait section should emit 12240×15840; landscape 15840×12240.
    REQUIRE(xml.contains(QStringLiteral("w:w=\"12240\"")));
    REQUIRE(xml.contains(QStringLiteral("w:w=\"15840\"")));
    REQUIRE(xml.contains(QStringLiteral("<w:titlePg/>")));
    REQUIRE(xml.contains(QStringLiteral("w:type w:val=\"nextPage\"")));
    CHECK(s.count() == 2);
}

TEST_CASE("SectionStack: continuous, even-page, odd-page break tags", "[v0.5.0][sections]")
{
    SectionStack s;
    for (auto k : { PageSectionBreak::Continuous,
                    PageSectionBreak::EvenPage,
                    PageSectionBreak::OddPage }) {
        SectionData sd;
        sd.breakKind = k;
        s.append(sd);
    }
    const QString xml = s.serializeForDocx();
    REQUIRE(xml.contains(QStringLiteral("w:type w:val=\"continuous\"")));
    REQUIRE(xml.contains(QStringLiteral("w:type w:val=\"evenPage\"")));
    REQUIRE(xml.contains(QStringLiteral("w:type w:val=\"oddPage\"")));
}

// ══════════════════════════════════════════════════════════════════════════
//  Full DOCX round-trip: paragraph + page style + section + columns +
//                         line numbering + drop cap + protection
// ══════════════════════════════════════════════════════════════════════════

TEST_CASE("Docx round-trip survives v0.5.0 metadata end-to-end", "[v0.5.0][roundtrip]")
{
    DocxExporter exporter;
    DocxImporter importer;

    // Build a body that exercises every feature above
    QJsonObject body = QJsonObject();
    QJsonArray paragraphs;
    paragraphs.append(makeParagraph(QStringLiteral("Hello, v0.5.0!")));
    body.insert(QStringLiteral("type"), QStringLiteral("doc"));
    body.insert(QStringLiteral("content"), paragraphs);

    QJsonObject vela;
    vela.insert(QStringLiteral("evenAndOddHeaders"), true);
    vela.insert(QStringLiteral("mirrorMargins"),      true);

    // Page styles
    QJsonArray pageStyles;
    {
        QJsonObject o;
        o.insert(QStringLiteral("id"), QStringLiteral("PageStyleDefault"));
        o.insert(QStringLiteral("xml"), QStringLiteral(
            "<w:style w:type=\"page\" w:styleId=\"PageStyleDefault\">"
            "<w:name w:val=\"Default\"/>"
            "<w:basedOn w:val=\"PageStyleDefault\"/>"
            "<w:pPr><w:pgSz w:w=\"12240\" w:h=\"15840\"/>"
            "<w:pgMar w:top=\"1440\" w:right=\"1440\" w:bottom=\"1440\" "
            "w:left=\"1440\" w:header=\"720\" w:footer=\"720\" w:gutter=\"0\"/>"
            "</w:pPr></w:style>"));
        pageStyles.append(o);
    }
    vela.insert(QStringLiteral("pageStyles"), pageStyles);

    // Sections: portrait + landscape with titlePg + columns
    QJsonArray sections;
    {
        QJsonObject s;
        s.insert(QStringLiteral("type"),  QStringLiteral("nextPage"));
        s.insert(QStringLiteral("orient"), 0);
        s.insert(QStringLiteral("top"),    1440);
        s.insert(QStringLiteral("right"),  1440);
        s.insert(QStringLiteral("bottom"), 1440);
        s.insert(QStringLiteral("left"),   1440);
        s.insert(QStringLiteral("titlePg"), false);
        s.insert(QStringLiteral("cols"),
            QStringLiteral("<w:cols w:num=\"2\" w:space=\"360\" w:equalWidth=\"1\" w:sep=\"1\"/>"));
        s.insert(QStringLiteral("lnNum"),
            QStringLiteral("<w:lnNumType w:countBy=\"1\" w:start=\"1\" w:distance=\"360\" w:restart=\"newPage\"/>"));
        sections.append(s);
    }
    vela.insert(QStringLiteral("sections"), sections);

    // Document protection
    vela.insert(QStringLiteral("documentProtection"),
        QStringLiteral("<w:documentProtection w:edit=\"readOnly\" w:enforcement=\"1\"/>"));

    body.insert(QStringLiteral("_vela"), vela);

    const QString path = tmpPath(QStringLiteral(".docx"));
    QString err;
    REQUIRE(exporter.exportToFile(path, body, err));
    CHECK(err.isEmpty());

    QJsonObject roundtrip;
    REQUIRE(importer.importFromFile(path, roundtrip, err));

    // _vela metadata must round-trip
    REQUIRE(roundtrip.contains(QStringLiteral("_vela")));
    const QJsonObject rtVela = roundtrip.value(QStringLiteral("_vela")).toObject();
    CHECK(rtVela.value(QStringLiteral("evenAndOddHeaders")).toBool(false) == true);
    CHECK(rtVela.value(QStringLiteral("mirrorMargins")).toBool(false)      == true);
    CHECK(rtVela.contains(QStringLiteral("pageStyles")));
    CHECK(rtVela.contains(QStringLiteral("sections")));

    REQUIRE(rtVela.contains(QStringLiteral("pageStyles")));
    const QJsonArray ps = rtVela.value(QStringLiteral("pageStyles")).toArray();
    CHECK(ps.size() == 1);
    const QString psId = ps[0].toObject().value(QStringLiteral("id")).toString();
    CHECK(psId == QStringLiteral("PageStyleDefault"));

    REQUIRE(rtVela.contains(QStringLiteral("sections")));
    const QJsonArray sec = rtVela.value(QStringLiteral("sections")).toArray();
    CHECK(sec.size() == 1);
    const QJsonObject firstSec = sec[0].toObject();
    CHECK(firstSec.value(QStringLiteral("top")).toInt(0) == 1440);
    CHECK(firstSec.value(QStringLiteral("orient")).toInt(0) == 0);

    REQUIRE(rtVela.contains(QStringLiteral("documentProtection")));
    const QString prot = rtVela.value(QStringLiteral("documentProtection")).toString();
    CAPTURE(prot);
    REQUIRE(prot.contains(QStringLiteral("w:edit=\"readOnly\"")));
    REQUIRE(prot.contains(QStringLiteral("w:enforcement=\"1\"")));

    QFile::remove(path);
}

TEST_CASE("Docx round-trip: drop cap survives in run properties", "[v0.5.0][roundtrip]")
{
    DocxExporter exporter;
    DocxImporter importer;

    QJsonObject p;
    QJsonArray content;
    QJsonObject r;
    r.insert(QStringLiteral("type"),  QStringLiteral("text"));
    r.insert(QStringLiteral("text"),  QStringLiteral("T"));
    QJsonObject dc;
    dc.insert(QStringLiteral("type"),  QStringLiteral("drop"));
    dc.insert(QStringLiteral("lines"), 3);
    r.insert(QStringLiteral("_dropCap"), dc);
    content.append(r);
    p.insert(QStringLiteral("type"),    QStringLiteral("paragraph"));
    p.insert(QStringLiteral("content"), content);

    QJsonObject body;
    QJsonArray paras;
    paras.append(p);
    body.insert(QStringLiteral("type"),    QStringLiteral("doc"));
    body.insert(QStringLiteral("content"), paras);

    const QString path = tmpPath(QStringLiteral(".docx"));
    QString err;
    REQUIRE(exporter.exportToFile(path, body, err));

    QJsonObject rt;
    REQUIRE(importer.importFromFile(path, rt, err));
    const QJsonArray ps = rt.value(QStringLiteral("content")).toArray();
    REQUIRE(ps.size() == 1);
    const QJsonArray runs = ps[0].toObject().value(QStringLiteral("content")).toArray();
    REQUIRE(runs.size() == 1);
    const QJsonObject firstRun = runs[0].toObject();
    REQUIRE(firstRun.contains(QStringLiteral("_dropCap")));
    CHECK(firstRun.value(QStringLiteral("_dropCap")).toObject()
            .value(QStringLiteral("lines")).toInt() == 3);
    QFile::remove(path);
}

TEST_CASE("Docx round-trip: legacy v0.3.x documents with no _vela block still import", "[v0.5.0][roundtrip]")
{
    // A paragraph body alone (no _vela) — the v0.5.0 importer must not break.
    DocxExporter exporter;
    DocxImporter importer;

    QJsonObject body;
    QJsonArray paras;
    paras.append(makeParagraph(QStringLiteral("Legacy paragraph")));
    body.insert(QStringLiteral("type"),    QStringLiteral("doc"));
    body.insert(QStringLiteral("content"), paras);

    const QString path = tmpPath(QStringLiteral(".docx"));
    QString err;
    REQUIRE(exporter.exportToFile(path, body, err));

    QJsonObject rt;
    REQUIRE(importer.importFromFile(path, rt, err));
    CHECK_FALSE(rt.contains(QStringLiteral("_vela")));  // older shapes get no metadata
    CHECK(rt.value(QStringLiteral("content")).toArray().size() == 1);

    QFile::remove(path);
}
