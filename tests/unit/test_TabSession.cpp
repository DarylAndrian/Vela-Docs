// test_TabSession.cpp — v0.3.1 (CRIT-02 + CRIT-05)
#include <catch2/catch_test_macros.hpp>
#include "document/TabSession.h"

TEST_CASE("TabSession — create gives unique id and Untitled title", "[tabsession]") {
    auto a = vela::TabSession::create();
    auto b = vela::TabSession::create();
    CHECK(a->id() != b->id());
    CHECK(a->title() == QStringLiteral("Untitled"));
    CHECK_FALSE(a->isDirty());
}

TEST_CASE("TabSession — markDirty flips state", "[tabsession]") {
    auto s = vela::TabSession::create();
    s->markDirty();
    CHECK(s->isDirty());
}
