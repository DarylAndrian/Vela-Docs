// ════════════════════════════════════════════════════════════════════════════
//  test_docx.cpp — Round-trip tests for DocxImporter / DocxExporter
//
//  Fixtures use genuine ProseMirror / TipTap JSON — exactly what
//  `editor.getJSON()` produces. An earlier version of this file used a private
//  {"style":"Heading1", marks:["bold"]} schema that the editor never emits, so
//  the tests passed while real documents lost every heading and every mark.
// ════════════════════════════════════════════════════════════════════════════
#include <catch2/catch_test_macros.hpp>

#include "formats/DocxImporter.h"
#include "formats/DocxExporter.h"
#include "util/Zip.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>

using namespace vela;

namespace {

QJsonObject text(const QString& s, const QJsonArray& marks = {})
{
    QJsonObject n;
    n.insert(QStringLiteral("type"), QStringLiteral("text"));
    n.insert(QStringLiteral("text"), s);
    if (!marks.isEmpty()) n.insert(QStringLiteral("marks"), marks);
    return n;
}

QJsonObject mark(const QString& type, const QJsonObject& attrs = {})
{
    QJsonObject m;
    m.insert(QStringLiteral("type"), type);
    if (!attrs.isEmpty()) m.insert(QStringLiteral("attrs"), attrs);
    return m;
}

QJsonObject paragraph(const QJsonArray& content, const QJsonObject& attrs = {})
{
    QJsonObject n;
    n.insert(QStringLiteral("type"), QStringLiteral("paragraph"));
    if (!attrs.isEmpty()) n.insert(QStringLiteral("attrs"), attrs);
    if (!content.isEmpty()) n.insert(QStringLiteral("content"), content);
    return n;
}

QJsonObject heading(int level, const QString& s)
{
    QJsonObject attrs;
    attrs.insert(QStringLiteral("level"), level);
    QJsonObject n;
    n.insert(QStringLiteral("type"), QStringLiteral("heading"));
    n.insert(QStringLiteral("attrs"), attrs);
    n.insert(QStringLiteral("content"), QJsonArray{text(s)});
    return n;
}

/// Concatenates every text node in a subtree — used to assert content survived.
QString allText(const QJsonValue& node)
{
    QString out;
    if (node.isString()) return node.toString();
    const QJsonObject o = node.toObject();
    if (o.value(QStringLiteral("type")).toString() == QLatin1String("text"))
        out += o.value(QStringLiteral("text")).toString();
    for (const auto& c : o.value(QStringLiteral("content")).toArray()) out += allText(c);
    return out;
}

bool hasMark(const QJsonObject& node, const QString& type)
{
    for (const auto& m : node.value(QStringLiteral("marks")).toArray()) {
        if (m.isObject() && m.toObject().value(QStringLiteral("type")).toString() == type)
            return true;
    }
    return false;
}

/// First text node found anywhere in the subtree.
QJsonObject firstTextNode(const QJsonValue& node)
{
    const QJsonObject o = node.toObject();
    if (o.value(QStringLiteral("type")).toString() == QLatin1String("text")) return o;
    for (const auto& c : o.value(QStringLiteral("content")).toArray()) {
        const QJsonObject found = firstTextNode(c);
        if (!found.isEmpty()) return found;
    }
    return {};
}

QJsonObject makeSampleBody()
{
    QJsonArray blocks;

    blocks.append(paragraph({text(QStringLiteral("Hello, world!"))}));
    blocks.append(heading(1, QStringLiteral("This is a heading")));
    blocks.append(paragraph({
        text(QStringLiteral("Mixed "), {mark(QStringLiteral("italic"))}),
        text(QStringLiteral("formatting"), {mark(QStringLiteral("bold"))}),
    }));

    QJsonObject body;
    body.insert(QStringLiteral("type"),    QStringLiteral("doc"));
    body.insert(QStringLiteral("content"), blocks);
    return body;
}

QString uniqueTmpPath(const QString& suffix)
{
    return QDir::tempPath() + QStringLiteral("/vela-test-")
        + QString::number(QDateTime::currentMSecsSinceEpoch()) + suffix;
}

/// Export -> import and hand back the resulting body.
QJsonObject roundTrip(const QJsonObject& body, QString& err)
{
    DocxExporter exporter;
    DocxImporter importer;
    const QString path = uniqueTmpPath(QStringLiteral(".docx"));

    QJsonObject result;
    if (exporter.exportToFile(path, body, err))
        importer.importFromFile(path, result, err);
    QFile::remove(path);
    return result;
}

} // anon namespace

TEST_CASE("DocxExporter writes a valid OOXML zip", "[docx]")
{
    DocxExporter exporter;
    DocxImporter importer;

    const QString path = uniqueTmpPath(QStringLiteral(".docx"));
    QString err;
    REQUIRE(exporter.exportToFile(path, makeSampleBody(), err));
    CHECK(err.isEmpty());
    CHECK(QFileInfo::exists(path));
    CHECK(QFileInfo(path).size() > 100);

    QJsonObject roundtrip;
    REQUIRE(importer.importFromFile(path, roundtrip, err));
    CHECK(err.isEmpty());
    REQUIRE(roundtrip.contains(QStringLiteral("content")));

    const QJsonArray blocks = roundtrip.value(QStringLiteral("content")).toArray();
    REQUIRE(blocks.size() == 3);
    CHECK(allText(blocks[0]) == QStringLiteral("Hello, world!"));

    // Headings must come back as heading nodes carrying their level, not as
    // paragraphs with a private "style" key.
    const QJsonObject h = blocks[1].toObject();
    CHECK(h.value(QStringLiteral("type")).toString() == QStringLiteral("heading"));
    CHECK(h.value(QStringLiteral("attrs")).toObject()
            .value(QStringLiteral("level")).toInt() == 1);
    CHECK(allText(h) == QStringLiteral("This is a heading"));

    QFile::remove(path);
}

TEST_CASE("Docx round-trips XML-special characters without corrupting the file", "[docx][regression]")
{
    // escapeXml() used to be an identity function: '&' stayed '&', '<' stayed
    // '<' and '"' became a curly quote. Any document containing those produced
    // malformed OOXML that Word refused to open.
    const QString nasty = QStringLiteral("Tom & Jerry <b>\"quoted\"</b> it's 5 > 3");

    QJsonObject body;
    body.insert(QStringLiteral("type"), QStringLiteral("doc"));
    body.insert(QStringLiteral("content"),
                QJsonArray{paragraph({text(nasty)})});

    QString err;
    const QJsonObject back = roundTrip(body, err);
    REQUIRE(err.isEmpty());

    const QJsonArray blocks = back.value(QStringLiteral("content")).toArray();
    REQUIRE(blocks.size() == 1);
    CHECK(allText(blocks[0]) == nasty);
}

TEST_CASE("Docx round-trips character marks as objects", "[docx]")
{
    QJsonObject body;
    body.insert(QStringLiteral("type"), QStringLiteral("doc"));
    body.insert(QStringLiteral("content"), QJsonArray{paragraph({
        text(QStringLiteral("b"), {mark(QStringLiteral("bold"))}),
        text(QStringLiteral("i"), {mark(QStringLiteral("italic"))}),
        text(QStringLiteral("u"), {mark(QStringLiteral("underline"))}),
        text(QStringLiteral("s"), {mark(QStringLiteral("strike"))}),
    })});

    QString err;
    const QJsonObject back = roundTrip(body, err);
    REQUIRE(err.isEmpty());

    const QJsonArray content = back.value(QStringLiteral("content")).toArray()[0]
                                   .toObject().value(QStringLiteral("content")).toArray();
    REQUIRE(content.size() == 4);
    CHECK(hasMark(content[0].toObject(), QStringLiteral("bold")));
    CHECK(hasMark(content[1].toObject(), QStringLiteral("italic")));
    CHECK(hasMark(content[2].toObject(), QStringLiteral("underline")));
    CHECK(hasMark(content[3].toObject(), QStringLiteral("strike")));
}

TEST_CASE("Docx round-trips bullet lists as list nodes", "[docx]")
{
    QJsonObject list;
    list.insert(QStringLiteral("type"), QStringLiteral("bulletList"));
    list.insert(QStringLiteral("content"), QJsonArray{
        [&] { QJsonObject i; i.insert(QStringLiteral("type"), QStringLiteral("listItem"));
              i.insert(QStringLiteral("content"), QJsonArray{paragraph({text(QStringLiteral("alpha"))})}); return i; }(),
        [&] { QJsonObject i; i.insert(QStringLiteral("type"), QStringLiteral("listItem"));
              i.insert(QStringLiteral("content"), QJsonArray{paragraph({text(QStringLiteral("beta"))})}); return i; }(),
    });

    QJsonObject body;
    body.insert(QStringLiteral("type"), QStringLiteral("doc"));
    body.insert(QStringLiteral("content"), QJsonArray{list});

    QString err;
    const QJsonObject back = roundTrip(body, err);
    REQUIRE(err.isEmpty());

    const QJsonArray blocks = back.value(QStringLiteral("content")).toArray();
    REQUIRE(blocks.size() == 1);
    const QJsonObject restored = blocks[0].toObject();
    CHECK(restored.value(QStringLiteral("type")).toString() == QStringLiteral("bulletList"));
    CHECK(allText(restored) == QStringLiteral("alphabeta"));
}

TEST_CASE("Docx round-trips tables", "[docx]")
{
    auto cell = [](const QString& s) {
        QJsonObject c;
        c.insert(QStringLiteral("type"), QStringLiteral("tableCell"));
        QJsonObject attrs;
        attrs.insert(QStringLiteral("colspan"), 1);
        attrs.insert(QStringLiteral("rowspan"), 1);
        c.insert(QStringLiteral("attrs"), attrs);
        c.insert(QStringLiteral("content"), QJsonArray{paragraph({text(s)})});
        return c;
    };
    auto row = [&](const QString& a, const QString& b) {
        QJsonObject r;
        r.insert(QStringLiteral("type"), QStringLiteral("tableRow"));
        r.insert(QStringLiteral("content"), QJsonArray{cell(a), cell(b)});
        return r;
    };

    QJsonObject table;
    table.insert(QStringLiteral("type"), QStringLiteral("table"));
    table.insert(QStringLiteral("content"),
                 QJsonArray{row(QStringLiteral("a1"), QStringLiteral("b1")),
                            row(QStringLiteral("a2"), QStringLiteral("b2"))});

    QJsonObject body;
    body.insert(QStringLiteral("type"), QStringLiteral("doc"));
    body.insert(QStringLiteral("content"), QJsonArray{table});

    QString err;
    const QJsonObject back = roundTrip(body, err);
    REQUIRE(err.isEmpty());

    const QJsonArray blocks = back.value(QStringLiteral("content")).toArray();
    REQUIRE(!blocks.isEmpty());
    const QJsonObject restored = blocks[0].toObject();
    CHECK(restored.value(QStringLiteral("type")).toString() == QStringLiteral("table"));
    CHECK(restored.value(QStringLiteral("content")).toArray().size() == 2);
    CHECK(allText(restored) == QStringLiteral("a1b1a2b2"));
}

TEST_CASE("Docx round-trips hyperlinks", "[docx]")
{
    const QString href = QStringLiteral("https://example.com/a?b=1&c=2");
    QJsonObject body;
    body.insert(QStringLiteral("type"), QStringLiteral("doc"));
    body.insert(QStringLiteral("content"), QJsonArray{paragraph({
        text(QStringLiteral("click"), {mark(QStringLiteral("link"),
             [&] { QJsonObject a; a.insert(QStringLiteral("href"), href); return a; }())}),
    })});

    QString err;
    const QJsonObject back = roundTrip(body, err);
    REQUIRE(err.isEmpty());

    const QJsonObject node = firstTextNode(back.value(QStringLiteral("content")).toArray()[0]);
    REQUIRE(!node.isEmpty());
    CHECK(node.value(QStringLiteral("text")).toString() == QStringLiteral("click"));
    REQUIRE(hasMark(node, QStringLiteral("link")));
    for (const auto& m : node.value(QStringLiteral("marks")).toArray()) {
        if (m.toObject().value(QStringLiteral("type")).toString() == QLatin1String("link")) {
            CHECK(m.toObject().value(QStringLiteral("attrs")).toObject()
                    .value(QStringLiteral("href")).toString() == href);
        }
    }
}

TEST_CASE("Docx round-trips paragraph alignment", "[docx]")
{
    QJsonObject attrs;
    attrs.insert(QStringLiteral("textAlign"), QStringLiteral("center"));

    QJsonObject body;
    body.insert(QStringLiteral("type"), QStringLiteral("doc"));
    body.insert(QStringLiteral("content"),
                QJsonArray{paragraph({text(QStringLiteral("centred"))}, attrs)});

    QString err;
    const QJsonObject back = roundTrip(body, err);
    REQUIRE(err.isEmpty());
    CHECK(back.value(QStringLiteral("content")).toArray()[0].toObject()
            .value(QStringLiteral("attrs")).toObject()
            .value(QStringLiteral("textAlign")).toString() == QStringLiteral("center"));
}

TEST_CASE("DocxExporter fails gracefully on unwritable path", "[docx]")
{
    DocxExporter exporter;
    QString err;
    CHECK_FALSE(exporter.exportToFile(
        QStringLiteral("Z:/this/path/should/never/exist.docx"), makeSampleBody(), err));
    CHECK_FALSE(err.isEmpty());
}

TEST_CASE("DocxImporter fails on missing file", "[docx]")
{
    DocxImporter importer;
    QJsonObject body;
    QString err;
    CHECK_FALSE(importer.importFromFile(QStringLiteral("Z:/nope.docx"), body, err));
    CHECK_FALSE(err.isEmpty());
}

TEST_CASE("Each ordered list gets its own numbering definition", "[docx][numbering]")
{
    auto list = [](int start) {
        QJsonObject attrs;
        attrs.insert(QStringLiteral("start"), start);
        QJsonObject item;
        item.insert(QStringLiteral("type"), QStringLiteral("listItem"));
        item.insert(QStringLiteral("content"),
                    QJsonArray{paragraph({text(QStringLiteral("one"))})});
        QJsonObject l;
        l.insert(QStringLiteral("type"), QStringLiteral("orderedList"));
        l.insert(QStringLiteral("attrs"), attrs);
        l.insert(QStringLiteral("content"), QJsonArray{item});
        return l;
    };

    QJsonObject body;
    body.insert(QStringLiteral("type"), QStringLiteral("doc"));
    body.insert(QStringLiteral("content"), QJsonArray{list(1), list(5)});

    DocxExporter exporter;
    QString err;
    const QString path = uniqueTmpPath(QStringLiteral(".docx"));
    REQUIRE(exporter.exportToFile(path, body, err));

    QHash<QString, QByteArray> entries;
    REQUIRE(Zip::openForReading(path, entries));
    QFile::remove(path);

    // Sharing one numId makes Word continue the second list from the first.
    const QString numbering = QString::fromUtf8(entries.value(QStringLiteral("word/numbering.xml")));
    REQUIRE_FALSE(numbering.isEmpty());
    CHECK(numbering.count(QStringLiteral("<w:abstractNum ")) == 2);
    CHECK(numbering.count(QStringLiteral("<w:num ")) == 2);
    CHECK(numbering.contains(QStringLiteral("<w:start w:val=\"5\"/>")));

    const QString doc = QString::fromUtf8(entries.value(QStringLiteral("word/document.xml")));
    CHECK(doc.contains(QStringLiteral("<w:numId w:val=\"1\"/>")));
    CHECK(doc.contains(QStringLiteral("<w:numId w:val=\"2\"/>")));
}

TEST_CASE("Docx round-trips per-column table widths", "[docx]")
{
    auto cell = [](int widthPx) {
        QJsonObject attrs;
        attrs.insert(QStringLiteral("colspan"), 1);
        attrs.insert(QStringLiteral("rowspan"), 1);
        attrs.insert(QStringLiteral("colwidth"), QJsonArray{widthPx});
        QJsonObject c;
        c.insert(QStringLiteral("type"), QStringLiteral("tableCell"));
        c.insert(QStringLiteral("attrs"), attrs);
        c.insert(QStringLiteral("content"), QJsonArray{paragraph({text(QStringLiteral("x"))})});
        return c;
    };

    QJsonObject row;
    row.insert(QStringLiteral("type"), QStringLiteral("tableRow"));
    row.insert(QStringLiteral("content"), QJsonArray{cell(200), cell(400)});

    QJsonObject table;
    table.insert(QStringLiteral("type"), QStringLiteral("table"));
    table.insert(QStringLiteral("content"), QJsonArray{row});

    QJsonObject body;
    body.insert(QStringLiteral("type"), QStringLiteral("doc"));
    body.insert(QStringLiteral("content"), QJsonArray{table});

    QString err;
    const QJsonObject back = roundTrip(body, err);
    REQUIRE(err.isEmpty());

    const QJsonArray rows = back.value(QStringLiteral("content")).toArray();
    REQUIRE(!rows.isEmpty());
    const QJsonObject restored = rows[0].toObject();
    REQUIRE(restored.value(QStringLiteral("type")).toString() == QStringLiteral("table"));
    const QJsonArray cells = restored.value(QStringLiteral("content")).toArray()
        .at(0).toObject().value(QStringLiteral("content")).toArray();
    REQUIRE(cells.size() == 2);

    // 200px -> 3000 twips -> 200px, so the widths must survive exactly.
    const auto width = [&cells](int i) {
        return cells.at(i).toObject().value(QStringLiteral("attrs")).toObject()
            .value(QStringLiteral("colwidth")).toArray().at(0).toInt();
    };
    CHECK(width(0) == 200);
    CHECK(width(1) == 400);
}
