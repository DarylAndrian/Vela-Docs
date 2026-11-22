// test_Document.cpp — v0.3.1 (CRIT-05)
#include <catch2/catch_test_macros.hpp>
#include "document/Document.h"

TEST_CASE("Document — fromPlainText round-trip", "[document]") {
    Document d; d.fromPlainText("hello\nworld");
    CHECK(d.toPlainText().contains("hello"));
    CHECK(d.toPlainText().contains("world"));
}

TEST_CASE("Document — dirty flag toggles", "[document]") {
    Document d;
    CHECK_FALSE(d.isDirty());
    d.markDirty();
    CHECK(d.isDirty());
    d.clearDirty();
    CHECK_FALSE(d.isDirty());
}

TEST_CASE("Document — UUID stable across saves", "[document]") {
    Document a, b;
    a.saveVersion("v1", {}); b.saveVersion("v1", {});
    CHECK(a.uuid() != b.uuid());
}
