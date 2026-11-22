// test_SpellChecker.cpp — v0.3.1 stub; v0.3.2 fleshes out Hunspell.
#include <catch2/catch_test_macros.hpp>
#include "services/SpellChecker.h"

TEST_CASE("SpellChecker — instance constructs", "[spell]") {
    SpellChecker sc;
    CHECK_FALSE(sc.isReady()); // not yet wired to Hunspell
}

TEST_CASE("SpellChecker — empty input returns zero suggestions", "[spell]") {
    SpellChecker sc;
    CHECK(sc.suggestions(QString()).empty());
}
