// test_SettingsStore.cpp — v0.3.1 (CRIT-05)
#include <catch2/catch_test_macros.hpp>
#include "services/SettingsStore.h"

TEST_CASE("SettingsStore — round-trip string", "[settings]") {
    SettingsStore s;
    s.setValue("test/key", QStringLiteral("hello"));
    CHECK(s.value("test/key").toString() == "hello");
}

TEST_CASE("SettingsStore — int default", "[settings]") {
    SettingsStore s;
    s.clear(); s.setValue("missing/int", 7);
    CHECK(s.value("missing/int", 99).toInt() == 7);
}
