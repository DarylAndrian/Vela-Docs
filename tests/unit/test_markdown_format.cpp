// test_markdown_format.cpp — v0.3.1 (CRIT-04 round 1)
//
// Catch2 tests for MarkdownImporter. The MarkdownExporter was deferred to
// v0.3.5 (POLISH-03). cmark-gfm is the underlying parser.

#include <catch2/catch_test_macros.hpp>
#include "document/Document.h"
#include "formats/MarkdownImporter.h"
#include <QTemporaryFile>

namespace { void write(const QString& s, QTemporaryFile& f) {
    f.setAutoRemove(false); f.open(); f.write(s.toUtf8()); f.close();
} }

TEST_CASE("MarkdownImporter — heading", "[md]") {
    MarkdownImporter imp; Document d;
    QTemporaryFile f; write("# Title\n\nbody\n", f);
    REQUIRE(imp.import(f.fileName(), &d));
    CHECK(d.toPlainText().contains("Title"));
    CHECK(d.toPlainText().contains("body"));
}

TEST_CASE("MarkdownImporter — bold/italic", "[md]") {
    MarkdownImporter imp; Document d;
    QTemporaryFile f; write("**bold** and *italic* here\n", f);
    REQUIRE(imp.import(f.fileName(), &d));
    CHECK(d.toPlainText().contains("bold"));
    CHECK(d.toPlainText().contains("italic"));
}

TEST_CASE("MarkdownImporter — list", "[md]") {
    MarkdownImporter imp; Document d;
    QTemporaryFile f; write("- A\n- B\n- C\n", f);
    REQUIRE(imp.import(f.fileName(), &d));
    CHECK(d.toPlainText().contains("A"));
    CHECK(d.toPlainText().contains("B"));
    CHECK(d.toPlainText().contains("C"));
}

TEST_CASE("MarkdownImporter — link", "[md]") {
    MarkdownImporter imp; Document d;
    QTemporaryFile f; write("click [here](https://example.com)\n", f);
    REQUIRE(imp.import(f.fileName(), &d));
    CHECK(d.toPlainText().contains("click"));
    CHECK(d.toPlainText().contains("here"));
}

TEST_CASE("MarkdownImporter — code block", "[md]") {
    MarkdownImporter imp; Document d;
    QTemporaryFile f; write("```\nprint('hi')\n```\n", f);
    REQUIRE(imp.import(f.fileName(), &d));
    CHECK(d.toPlainText().contains("print"));
}

TEST_CASE("MarkdownImporter — empty file", "[md]") {
    MarkdownImporter imp; Document d;
    QTemporaryFile f; write("", f);
    // Empty file should still be considered "imported" without crashing.
    REQUIRE(imp.import(f.fileName(), &d));
}
